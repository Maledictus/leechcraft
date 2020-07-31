/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "picasaaccount.h"
#include <QDomDocument>
#include <QInputDialog>
#include <QMainWindow>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardItemModel>
#include <QtDebug>
#include <QUuid>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/ientitymanager.h>
#include <util/sll/queuemanager.h>
#include <util/xpc/util.h>
#include "albumsettingsdialog.h"
#include "picasaservice.h"
#include "uploadmanager.h"

namespace LC
{
namespace Blasq
{
namespace Vangog
{
	PicasaAccount::PicasaAccount (const QString& name, PicasaService *service,
			ICoreProxy_ptr proxy, const QString& login, const QByteArray& id)
	: QObject (service)
	, Name_ (name)
	, Service_ (service)
	, Proxy_ (proxy)
	, ID_ (id.isEmpty () ? QUuid::createUuid ().toByteArray () : id)
	, Login_ (login)
	, Ready_ (false)
	, PicasaManager_ (new PicasaManager (this, this))
	, CollectionsModel_ (new NamedModel<QStandardItemModel> (this))
	, AllPhotosItem_ (0)
	, RequestQueue_ (new Util::QueueManager (350, this))
	, UploadManager_ (new UploadManager (RequestQueue_, Proxy_, this))
	{
		CollectionsModel_->setHorizontalHeaderLabels ({ tr ("Name") });

		connect (PicasaManager_,
				SIGNAL (gotAlbums (QList<Album>)),
				this,
				SLOT (handleGotAlbums (QList<Album>)));
		connect (PicasaManager_,
				SIGNAL (gotAlbum (Album)),
				this,
				SLOT (handleGotAlbum (Album)));
		connect (PicasaManager_,
				SIGNAL (gotPhotos (QList<Photo>)),
				this,
				SLOT (handleGotPhotos (QList<Photo>)));
		connect (PicasaManager_,
				SIGNAL (gotPhoto (Photo)),
				this,
				SLOT (handleGotPhoto (Photo)));
		connect (PicasaManager_,
				SIGNAL (deletedPhoto (QByteArray)),
				this,
				SLOT (handleDeletedPhotos (QByteArray)));
		connect (PicasaManager_,
				SIGNAL (gotError (int, QString)),
				this,
				SLOT (handleGotError (int, QString)));

		connect (UploadManager_,
				SIGNAL (gotError (int, QString)),
				this,
				SLOT (handleGotError (int, QString)));
	}

	ICoreProxy_ptr PicasaAccount::GetProxy () const
	{
		return Proxy_;
	}

	void PicasaAccount::Release ()
	{
		emit accountChanged (this);
	}

	QByteArray PicasaAccount::Serialize () const
	{
		QByteArray result;
		{
			QDataStream out (&result, QIODevice::WriteOnly);
			out << static_cast<quint8> (4)
					<< Name_
					<< RefreshToken_
					<< Login_
					<< ID_
					<< PicasaManager_->GetAccessToken ()
					<< PicasaManager_->GetAccessTokenExpireDate ();
		}
		return result;
	}

	PicasaAccount* PicasaAccount::Deserialize (const QByteArray& ba,
			PicasaService *service, ICoreProxy_ptr proxy)
	{
		QDataStream in (ba);

		quint8 version = 0;
		in >> version;
		if (version < 1 || version > 4)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return nullptr;
		}

		QString name;
		QString refreshKey;
		QString login;
		QByteArray id;
		in >> name
				>> refreshKey;
		if (version >= 2)
			in >> login;

		if (version >= 3)
			in >> id;

		auto acc = new PicasaAccount (name, service, proxy, login, id);
		if (version == 4)
			in >> acc->AccessToken_
					>> acc->AccessTokenExpireDate_;

		acc->RefreshToken_ = refreshKey;

		return acc;
	}

	void PicasaAccount::Schedule (std::function<void (QString)> func)
	{
		PicasaManager_->Schedule (func);
	}

	QObject* PicasaAccount::GetQObject ()
	{
		return this;
	}

	IService* PicasaAccount::GetService () const
	{
		return Service_;
	}

	QString PicasaAccount::GetName () const
	{
		return Name_;
	}

	QByteArray PicasaAccount::GetID () const
	{
		return ID_;
	}

	QString PicasaAccount::GetLogin () const
	{
		return Login_;
	}

	QString PicasaAccount::GetAccessToken () const
	{
		return AccessToken_;
	}

	QDateTime PicasaAccount::GetAccessTokenExpireDate () const
	{
		return AccessTokenExpireDate_;
	}

	void PicasaAccount::SetRefreshToken (const QString& token)
	{
		RefreshToken_ = token;
	}

	QString PicasaAccount::GetRefreshToken () const
	{
		return RefreshToken_;
	}

	QAbstractItemModel* PicasaAccount::GetCollectionsModel () const
	{
		return CollectionsModel_;
	}

	void PicasaAccount::UpdateCollections ()
	{
		if (TryToEnterLoginIfNoExists ())
		{
			AlbumId2AlbumItem_.clear ();
			AlbumID2PhotosSet_.clear ();
			if (int rowCount = CollectionsModel_->rowCount ())
				CollectionsModel_->removeRows (0, rowCount);
			PicasaManager_->UpdateCollections ();
		}
	}

	void PicasaAccount::Delete (const QModelIndex& index)
	{
		switch (index.data (CollectionRole::Type).toInt ())
		{
		case ItemType::Collection:
			PicasaManager_->DeleteAlbum (index.data (CollectionRole::ID).toByteArray ());
			break;
		case ItemType::Image:
		{
			const auto& id = index.data (CollectionRole::ID).toByteArray ();
			DeletedPhotoId2Index_ [id] = index;
			PicasaManager_->DeletePhoto (id, index.data (AlbumId).toByteArray ());
			break;
		}
		case ItemType::AllPhotos:
		default:
			break;
		}
	}

	bool PicasaAccount::SupportsFeature (DeleteFeature feature) const
	{
		switch (feature)
		{
		case DeleteFeature::DeleteImages:
			return true;
		case DeleteFeature::DeleteCollections:
			return true;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown feature"
				<< static_cast<int> (feature);
		return false;
	}

	void PicasaAccount::CreateCollection (const QModelIndex&)
	{
		AlbumSettingsDialog dia ({}, Proxy_);
		if (dia.exec () != QDialog::Accepted)
			return;

		PicasaManager_->CreateAlbum (dia.GetName (),
				dia.GetDesc (), dia.GetPrivacyLevel ());
	}

	bool PicasaAccount::HasUploadFeature (ISupportUploads::Feature feature) const
	{
		switch (feature)
		{
		case Feature::RequiresAlbumOnUpload:
			return false;
		case Feature::SupportsDescriptions:
			return true;
		}

		return false;
	}

	void PicasaAccount::UploadImages (const QModelIndex& collection, const QList<UploadItem>& paths)
	{
		const auto& albumId = collection.data (CollectionRole::ID).toByteArray ();
		UploadManager_->Upload (albumId, paths);
	}

	void PicasaAccount::ImageUploadResponse (const QByteArray& content, const UploadItem& item)
	{
		const auto& photo = PicasaManager_->handleImageUploaded (content);
		emit itemUploaded (item, photo.Url_);
	}

	bool PicasaAccount::TryToEnterLoginIfNoExists ()
	{
		if (Login_.isEmpty ())
		{
			const auto& text = QInputDialog::getText (Proxy_->GetRootWindowsManager ()->GetPreferredWindow (),
					"LeechCraft",
					tr ("Enter your Google login to access to Picasa Web Albums from %1 account:")
						.arg ("<em>" + Name_ + "</em>"));

			if (!text.isEmpty ())
			{
				Login_ = text;
				emit accountChanged (this);
				return true;
			}
			else
				return false;
		}

		return true;
	}

	void PicasaAccount::CreatePhotoItem (const Photo& photo)
	{
		auto mkItem = [&photo, this] () -> QStandardItem*
		{
			auto item = new QStandardItem (photo.Title_);
			item->setEditable (false);
			item->setData (ItemType::Image, CollectionRole::Type);
			item->setData (photo.ID_, CollectionRole::ID);
			item->setData (photo.Title_, CollectionRole::Name);

			item->setData (photo.Url_, CollectionRole::Original);
			item->setData (QSize (photo.Width_, photo.Height_), CollectionRole::OriginalSize);
			if (!photo.Thumbnails_.isEmpty ())
			{
				auto first = photo.Thumbnails_.first ();
				auto last = photo.Thumbnails_.last ();
				item->setData (first.Url_, CollectionRole::SmallThumb);
				item->setData (QSize (first.Width_, first.Height_), CollectionRole::SmallThumbSize);
				item->setData (last.Url_, CollectionRole::MediumThumb);
				item->setData (QSize (last.Width_, last.Height_), CollectionRole::MediumThumbSize);
			}

			item->setData (photo.AlbumID_, AlbumId);
			Item2PhotoId_ [item] = photo.ID_;

			return item;
		};

		AllPhotosItem_->appendRow (mkItem ());

		if (!AlbumId2AlbumItem_.contains (photo.AlbumID_))
			return;

		if (AlbumID2PhotosSet_ [photo.AlbumID_].contains (photo.ID_))
			return;

		AlbumID2PhotosSet_ [photo.AlbumID_] << photo.ID_;
		AlbumId2AlbumItem_ [photo.AlbumID_]->appendRow (mkItem ());
	}

	void PicasaAccount::handleGotAlbums (const QList<Album>& albums)
	{
		CollectionsModel_->setHorizontalHeaderLabels ({ tr ("Name") });

		AlbumId2AlbumItem_.clear ();

		AllPhotosItem_ = new QStandardItem (tr ("All photos"));
		AllPhotosItem_->setData (ItemType::AllPhotos, CollectionRole::Type);
		AllPhotosItem_->setEditable (false);
		CollectionsModel_->appendRow (AllPhotosItem_);

		for (const auto& album : albums)
		{
			auto item = new QStandardItem (album.Title_);
			item->setData (ItemType::Collection, CollectionRole::Type);
			item->setData (album.ID_, CollectionRole::ID);
			item->setEditable (false);
			AlbumId2AlbumItem_ [album.ID_] = item;
			CollectionsModel_->appendRow (item);
		}
	}

	void PicasaAccount::handleGotAlbum (const Album& album)
	{
		auto item = new QStandardItem (album.Title_);
		item->setData (ItemType::Collection, CollectionRole::Type);
		item->setData (album.ID_, CollectionRole::ID);
		item->setEditable (false);
		AlbumId2AlbumItem_ [album.ID_] = item;
		CollectionsModel_->appendRow (item);
		Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq",
				tr ("Album was created successfully"), Priority::Info));
	}

	void PicasaAccount::handleGotPhotos (const QList<Photo>& photos)
	{
		for (const auto& photo : photos)
			CreatePhotoItem (photo);
		emit doneUpdating ();
	}

	void PicasaAccount::handleGotPhoto (const Photo& photo)
	{
		CreatePhotoItem (photo);
		emit doneUpdating ();
		Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq",
				tr ("Image was uploaded successfully"), Priority::Info));
	}

	void PicasaAccount::handleDeletedPhotos (const QByteArray& photoId)
	{
		if (!DeletedPhotoId2Index_.contains (photoId))
			return;

		auto index = DeletedPhotoId2Index_.take (photoId);
		if (!index.isValid ())
			return;

		const auto albumId = index.data (AlbumId).toByteArray ();
		CollectionsModel_->removeRow (index.row (), index.parent ());
		if (!AlbumId2AlbumItem_.contains (albumId))
			return;

		auto albumItem = AlbumId2AlbumItem_ [albumId];
		for (int i = 0, count = albumItem->rowCount (); i < count; ++i)
		{
			auto childItem = albumItem->child (i);
			if (childItem->data (CollectionRole::ID).toByteArray () == photoId)
			{
				CollectionsModel_->removeRow (i, albumItem->index ());
				break;
			}
		}
		emit doneUpdating ();
	}

	void PicasaAccount::handleGotError (int errorCode, const QString& errorString)
	{
		Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq",
				tr ("Error during operation: %1 (%2)")
					.arg (errorCode)
					.arg (errorString),
				Priority::Warning));
	}

}
}
}
