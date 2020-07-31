/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "vkaccount.h"
#include <QUuid>
#include <QStandardItemModel>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QtDebug>
#include <util/svcauth/vkauthmanager.h>
#include <util/sll/queuemanager.h>
#include <util/sll/urloperator.h>
#include "vkservice.h"
#include "albumsettingsdialog.h"
#include "uploadmanager.h"

namespace LC
{
namespace Blasq
{
namespace Rappor
{
	VkAccount::VkAccount (const QString& name, VkService *service, ICoreProxy_ptr proxy, const QByteArray& id, const QByteArray& cookies)
	: QObject (service)
	, Name_ (name)
	, ID_ (id.isEmpty () ? QUuid::createUuid ().toByteArray () : id)
	, Service_ (service)
	, Proxy_ (proxy)
	, CollectionsModel_ (new NamedModel<QStandardItemModel> (this))
	, AuthMgr_ (new Util::SvcAuth::VkAuthManager (name,
			"3762977", { "photos" }, cookies, proxy))
	, RequestQueue_ (new Util::QueueManager (350, this))
	, UploadManager_ (new UploadManager (RequestQueue_, Proxy_, this))
	{
		CollectionsModel_->setHorizontalHeaderLabels ({ tr ("Name") });

		AllPhotosItem_ = new QStandardItem (tr ("All photos"));
		AllPhotosItem_->setData (ItemType::AllPhotos, CollectionRole::Type);
		AllPhotosItem_->setEditable (false);
		CollectionsModel_->appendRow (AllPhotosItem_);

		connect (AuthMgr_,
				SIGNAL (cookiesChanged (QByteArray)),
				this,
				SLOT (handleCookies (QByteArray)));
		connect (AuthMgr_,
				SIGNAL (gotAuthKey (QString)),
				this,
				SLOT (handleAuthKey (QString)));

		connect (UploadManager_,
				SIGNAL (itemUploaded (UploadItem, QUrl)),
				this,
				SIGNAL (itemUploaded (UploadItem, QUrl)));
	}

	QByteArray VkAccount::Serialize () const
	{
		QByteArray result;
		{
			QDataStream out (&result, QIODevice::WriteOnly);
			out << static_cast<quint8> (1)
					<< Name_
					<< ID_
					<< LastCookies_;
		}
		return result;
	}

	VkAccount* VkAccount::Deserialize (const QByteArray& ba, VkService *service, ICoreProxy_ptr proxy)
	{
		QDataStream in (ba);

		quint8 version = 0;
		in >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return nullptr;
		}

		QString name;
		QByteArray id;
		QByteArray cookies;
		in >> name >> id >> cookies;

		return new VkAccount (name, service, proxy, id, cookies);
	}

	void VkAccount::Schedule (std::function<void (QString)> func)
	{
		CallQueue_ << func;
		AuthMgr_->GetAuthKey ();
	}

	QObject* VkAccount::GetQObject ()
	{
		return this;
	}

	IService* VkAccount::GetService () const
	{
		return Service_;
	}

	QString VkAccount::GetName () const
	{
		return Name_;
	}

	QByteArray VkAccount::GetID () const
	{
		return ID_;
	}

	QAbstractItemModel* VkAccount::GetCollectionsModel () const
	{
		return CollectionsModel_;
	}

	void VkAccount::UpdateCollections ()
	{
		if (IsUpdating_)
			return;

		IsUpdating_ = true;

		if (auto rc = AllPhotosItem_->rowCount ())
			AllPhotosItem_->removeRows (0, rc);

		auto rootRc = CollectionsModel_->rowCount ();
		if (rootRc > 1)
			CollectionsModel_->removeRows (1, rootRc - 1);

		Albums_.clear ();

		CallQueue_.append ([this] (const QString& authKey) -> void
			{
				QUrl albumsUrl ("https://api.vk.com/method/photos.getAlbums.xml");
				Util::UrlOperator { albumsUrl }
						("access_token", authKey);
				RequestQueue_->Schedule ([this, albumsUrl]
					{
						connect (Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (albumsUrl)),
								SIGNAL (finished ()),
								this,
								SLOT (handleGotAlbums ()));
					}, this);

				QUrl photosUrl ("https://api.vk.com/method/photos.getAll.xml");
				Util::UrlOperator { photosUrl }
						("access_token", authKey)
						("count", "100")
						("photo_sizes", "1");
				RequestQueue_->Schedule ([this, photosUrl]
					{
						connect (Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (photosUrl)),
								SIGNAL (finished ()),
								this,
								SLOT (handleGotPhotos ()));
					}, this);
			});

		AuthMgr_->GetAuthKey ();
	}

	bool VkAccount::HasUploadFeature (Feature feature) const
	{
		switch (feature)
		{
		case Feature::RequiresAlbumOnUpload:
		case Feature::SupportsDescriptions:
			return true;
		}

		return false;
	}

	void VkAccount::CreateCollection (const QModelIndex&)
	{
		AlbumSettingsDialog dia ({}, Proxy_);
		if (dia.exec () != QDialog::Accepted)
			return;

		const struct
		{
			QString Name_;
			QString Desc_;
			int Priv_;
			int CommentPriv_;
		} params
		{
			dia.GetName (),
			dia.GetDesc (),
			dia.GetPrivacyLevel (),
			dia.GetCommentsPrivacyLevel ()
		};

		CallQueue_.append ([this, params] (const QString& authKey) -> void
			{
				QUrl createUrl ("https://api.vk.com/method/photos.createAlbum.xml");
				Util::UrlOperator { createUrl }
						("title", params.Name_)
						("description", params.Desc_)
						("privacy", QString::number (params.Priv_))
						("comment_privacy", QString::number (params.CommentPriv_))
						("access_token", authKey);
				RequestQueue_->Schedule ([this, createUrl]
					{
						connect (Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (createUrl)),
								SIGNAL (finished ()),
								this,
								SLOT (handleAlbumCreated ()));
					}, this);
			});

		AuthMgr_->GetAuthKey ();
	}

	void VkAccount::UploadImages (const QModelIndex& collection, const QList<UploadItem>& items)
	{
		const auto& aidStr = collection.data (CollectionRole::ID).toString ();
		UploadManager_->Upload (aidStr, items);
	}

	bool VkAccount::SupportsFeature (DeleteFeature feature) const
	{
		switch (feature)
		{
		case DeleteFeature::DeleteImages:
			return true;
		case DeleteFeature::DeleteCollections:
			return false;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown feature"
				<< static_cast<int> (feature);
		return false;
	}

	void VkAccount::Delete (const QModelIndex& item)
	{
		const auto type = item.data (CollectionRole::Type).toInt ();
		const auto& id = item.data (CollectionRole::ID).toString ();
		switch (type)
		{
		case ItemType::AllPhotos:
			break;
		case ItemType::Collection:
		{
			break;
		}
		case ItemType::Image:
			CallQueue_.append ([this, id] (const QString& authKey) -> void
				{
					QUrl delUrl ("https://api.vk.com/method/photos.delete.xml");
					Util::UrlOperator { delUrl }
							("pid", id)
							("access_token", authKey);
					RequestQueue_->Schedule ([this, delUrl] () -> void
						{
							auto reply = Proxy_->GetNetworkAccessManager ()->
									get (QNetworkRequest (delUrl));
							connect (reply,
									SIGNAL (finished ()),
									reply,
									SLOT (deleteLater ()));
						}, this);
				});
			AuthMgr_->GetAuthKey ();

			CollectionsModel_->removeRow (item.row (), item.parent ());
			for (const auto& albumItem : Albums_)
				for (int i = 0; i < albumItem->rowCount (); ++i)
				{
					const auto subItem = albumItem->child (i, 0);
					if (subItem->data (CollectionRole::ID).toString () == id)
					{
						albumItem->removeRow (subItem->row ());
						break;
					}
				}
			break;
		}
	}

	void VkAccount::HandleAlbumElement (const QDomElement& albumElem)
	{
		const auto& title = albumElem.firstChildElement ("title").text ();
		auto item = new QStandardItem (title);
		item->setEditable (false);
		item->setData (ItemType::Collection, CollectionRole::Type);

		const auto& aidStr = albumElem.firstChildElement ("aid").text ();
		item->setData (aidStr, CollectionRole::ID);

		CollectionsModel_->appendRow (item);

		const auto aid = aidStr.toInt ();
		Albums_ [aid] = item;
	}

	bool VkAccount::HandlePhotoElement (const QDomElement& photoElem, bool atEnd)
	{
		auto mkItem = [&photoElem] () -> QStandardItem*
		{
			const auto& idText = photoElem.firstChildElement ("pid").text ();

			auto item = new QStandardItem (idText);
			item->setData (ItemType::Image, CollectionRole::Type);
			item->setData (idText, CollectionRole::ID);
			item->setData (idText, CollectionRole::Name);

			const auto& sizesElem = photoElem.firstChildElement ("sizes");
			auto getType = [&sizesElem] (const QString& type) -> QPair<QUrl, QSize>
			{
				auto sizeElem = sizesElem.firstChildElement ("size");
				while (!sizeElem.isNull ())
				{
					if (sizeElem.firstChildElement ("type").text () != type)
					{
						sizeElem = sizeElem.nextSiblingElement ("size");
						continue;
					}

					const auto& src = sizeElem.firstChildElement ("src").text ();
					const auto width = sizeElem.firstChildElement ("width").text ().toInt ();
					const auto height = sizeElem.firstChildElement ("height").text ().toInt ();

					return { src, { width, height } };
				}

				return {};
			};

			const auto& small = getType ("m");
			const auto& mid = getType ("x");
			auto orig = getType ("w");
			QStringList sizeCandidates { "z", "y", "x", "r" };
			while (orig.second.width () <= 0)
			{
				if (sizeCandidates.isEmpty ())
					return nullptr;

				orig = getType (sizeCandidates.takeFirst ());
			}

			item->setData (small.first, CollectionRole::SmallThumb);
			item->setData (small.second, CollectionRole::SmallThumbSize);

			item->setData (mid.first, CollectionRole::MediumThumb);
			item->setData (mid.second, CollectionRole::MediumThumbSize);

			item->setData (orig.first, CollectionRole::Original);
			item->setData (orig.second, CollectionRole::OriginalSize);

			return item;
		};

		auto allItem = mkItem ();
		if (!allItem)
			return false;

		if (atEnd)
			AllPhotosItem_->appendRow (allItem);
		else
			AllPhotosItem_->insertRow (0, allItem);

		const auto aid = photoElem.firstChildElement ("aid").text ().toInt ();
		if (Albums_.contains (aid))
		{
			auto album = Albums_ [aid];
			if (atEnd)
				album->appendRow (mkItem ());
			else
				album->insertRow (0, mkItem ());
		}

		return true;
	}

	void VkAccount::handleGotAlbums ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse reply"
					<< data;
			IsUpdating_ = false;
			return;
		}

		auto albumElem = doc
				.documentElement ()
				.firstChildElement ("album");
		while (!albumElem.isNull ())
		{
			HandleAlbumElement (albumElem);
			albumElem = albumElem.nextSiblingElement ("album");
		}
	}

	void VkAccount::handleAlbumCreated ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse reply"
					<< data;
			return;
		}

		HandleAlbumElement (doc.documentElement ().firstChildElement ("album"));
	}

	void VkAccount::handlePhotosInfosFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse reply"
					<< data;
			return;
		}

		auto photoElem = doc
				.documentElement ()
				.firstChildElement ("photo");
		while (!photoElem.isNull ())
		{
			HandlePhotoElement (photoElem, false);
			photoElem = photoElem.nextSiblingElement ("photo");
		}
	}

	void VkAccount::handleGotPhotos ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse reply"
					<< data;
			IsUpdating_ = false;
			return;
		}

		bool finishReached = false;

		auto photoElem = doc
				.documentElement ()
				.firstChildElement ("photo");
		while (!photoElem.isNull ())
		{
			if (!HandlePhotoElement (photoElem))
			{
				finishReached = true;
				break;
			}

			photoElem = photoElem.nextSiblingElement ("photo");
		}

		const auto count = doc.documentElement ().firstChildElement ("count").text ().toInt ();
		if (finishReached ||
				count == AllPhotosItem_->rowCount ())
		{
			IsUpdating_ = false;
			emit doneUpdating ();
			return;
		}

		const auto offset = AllPhotosItem_->rowCount ();

		CallQueue_.append ([this, offset] (const QString& authKey) -> void
			{
				QUrl photosUrl ("https://api.vk.com/method/photos.getAll.xml");
				Util::UrlOperator { photosUrl }
						("access_token", authKey)
						("count", "100")
						("offset", QString::number (offset))
						("photo_sizes", "1");
				RequestQueue_->Schedule ([this, photosUrl]
					{
						connect (Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (photosUrl)),
								SIGNAL (finished ()),
								this,
								SLOT (handleGotPhotos ()));
					}, this);
			});

		AuthMgr_->GetAuthKey ();
	}

	void VkAccount::handleAuthKey (const QString& authKey)
	{
		while (!CallQueue_.isEmpty ())
			CallQueue_.takeFirst () (authKey);
	}

	void VkAccount::handleCookies (const QByteArray& cookies)
	{
		LastCookies_ = cookies;
		emit accountChanged (this);
	}
}
}
}
