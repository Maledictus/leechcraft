/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "flickraccount.h"
#include <QTimer>
#include <QSettings>
#include <QCoreApplication>
#include <QUuid>
#include <QtDebug>
#include <QStandardItemModel>
#include <QDomDocument>
#include <interfaces/core/ientitymanager.h>
#include <util/xpc/util.h>
#include "flickrservice.h"

namespace LC
{
namespace Blasq
{
namespace Spegnersi
{
	const QString RequestTokenURL = "http://www.flickr.com/services/oauth/request_token";
	const QString UserAuthURL = "http://www.flickr.com/services/oauth/authorize";
	const QString AccessTokenURL = "http://www.flickr.com/services/oauth/access_token";

	FlickrAccount::FlickrAccount (const QString& name,
			FlickrService *service, ICoreProxy_ptr proxy, const QByteArray& id)
	: QObject (service)
	, Name_ (name)
	, ID_ (id.isEmpty () ? QUuid::createUuid ().toByteArray () : id)
	, Service_ (service)
	, Proxy_ (proxy)
	, Req_ (new KQOAuthRequest (this))
	, AuthMgr_ (new KQOAuthManager (this))
	, CollectionsModel_ (new NamedModel<QStandardItemModel> (this))
	{
		AuthMgr_->setNetworkManager (proxy->GetNetworkAccessManager ());
		AuthMgr_->setHandleUserAuthorization (true);

		connect (AuthMgr_,
				SIGNAL (temporaryTokenReceived (QString, QString)),
				this,
				SLOT (handleTempToken (QString, QString)));
		connect (AuthMgr_,
				SIGNAL (authorizationReceived (QString, QString)),
				this,
				SLOT (handleAuthorization (QString, QString)));
		connect (AuthMgr_,
				SIGNAL (accessTokenReceived (QString, QString)),
				this,
				SLOT (handleAccessToken (QString, QString)));

		connect (AuthMgr_,
				SIGNAL (requestReady (QByteArray)),
				this,
				SLOT (handleReply (QByteArray)));

		QTimer::singleShot (0,
				this,
				SLOT (checkAuthTokens ()));
	}

	QByteArray FlickrAccount::Serialize () const
	{
		QByteArray result;
		{
			QDataStream ostr (&result, QIODevice::WriteOnly);
			ostr << static_cast<quint8> (1)
					<< ID_
					<< Name_
					<< AuthToken_
					<< AuthSecret_;
		}
		return result;
	}

	FlickrAccount* FlickrAccount::Deserialize (const QByteArray& ba, FlickrService *service, ICoreProxy_ptr proxy)
	{
		QDataStream istr (ba);

		quint8 version = 0;
		istr >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return nullptr;
		}

		QByteArray id;
		QString name;
		istr >> id
				>> name;

		auto acc = new FlickrAccount (name, service, proxy, id);
		istr >> acc->AuthToken_
				>> acc->AuthSecret_;
		return acc;
	}

	QObject* FlickrAccount::GetQObject ()
	{
		return this;
	}

	IService* FlickrAccount::GetService () const
	{
		return Service_;
	}

	QString FlickrAccount::GetName () const
	{
		return Name_;
	}

	QByteArray FlickrAccount::GetID () const
	{
		return ID_;
	}

	QAbstractItemModel* FlickrAccount::GetCollectionsModel () const
	{
		return CollectionsModel_;
	}

	void FlickrAccount::UpdateCollections ()
	{
		switch (State_)
		{
		case State::CollectionsRequested:
			return;
		case State::Idle:
			break;
		default:
			CallQueue_ << [this] { UpdateCollections (); };
			return;
		}

		if (AuthToken_.isEmpty () || AuthSecret_.isEmpty ())
		{
			UpdateAfterAuth_ = true;
			return;
		}

		UpdateAfterAuth_ = false;

		auto req = MakeRequest (QString ("http://api.flickr.com/services/rest/"), KQOAuthRequest::AuthorizedRequest);
		req->setAdditionalParameters ({
					{ "method", "flickr.photos.search" },
					{ "user_id", "me" },
					{ "format", "rest" },
					{ "per_page", "500" },
					{ "extras", "url_o,url_z,url_m" }
				});
		AuthMgr_->executeRequest (req);

		CollectionsModel_->clear ();
		CollectionsModel_->setHorizontalHeaderLabels ({ tr ("Name") });

		AllPhotosItem_ = new QStandardItem (tr ("All photos"));
		AllPhotosItem_->setData (ItemType::AllPhotos, CollectionRole::Type);
		AllPhotosItem_->setEditable (false);
		CollectionsModel_->appendRow (AllPhotosItem_);

		State_ = State::CollectionsRequested;
	}

	KQOAuthRequest* FlickrAccount::MakeRequest (const QUrl& url, KQOAuthRequest::RequestType type)
	{
		Req_->clearRequest ();
		Req_->initRequest (type, url);
		Req_->setConsumerKey ("08efe88f972b2b89bd35e42bb26f970e");
		Req_->setConsumerSecretKey ("f70ac4b1ab7c499b");
		Req_->setSignatureMethod (KQOAuthRequest::HMAC_SHA1);

		if (!AuthToken_.isEmpty () && !AuthSecret_.isEmpty ())
		{
			Req_->setToken (AuthToken_);
			Req_->setTokenSecret (AuthSecret_);
		}

		return Req_;
	}

	void FlickrAccount::HandleCollectionsReply (const QByteArray& data)
	{
		qDebug () << Q_FUNC_INFO;
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse"
					<< data;
			State_ = State::Idle;
			return;
		}

		const auto& photos = doc
				.documentElement ()
				.firstChildElement ("photos");

		auto photo = photos.firstChildElement ("photo");
		while (!photo.isNull ())
		{
			auto item = new QStandardItem;

			item->setText (photo.attribute ("title"));
			item->setEditable (false);
			item->setData (ItemType::Image, CollectionRole::Type);
			item->setData (photo.attribute ("id"), CollectionRole::ID);
			item->setData (photo.attribute ("title"), CollectionRole::Name);

			auto getSize = [&photo] (const QString& s) -> QSize
			{
				return
				{
					photo.attribute ("width_" + s).toInt (),
					photo.attribute ("height_" + s).toInt ()
				};
			};

			item->setData (QUrl (photo.attribute ("url_o")), CollectionRole::Original);
			item->setData (getSize ("o"), CollectionRole::OriginalSize);
			item->setData (QUrl (photo.attribute ("url_z")), CollectionRole::MediumThumb);
			item->setData (getSize ("z"), CollectionRole::MediumThumbSize);
			item->setData (QUrl (photo.attribute ("url_m")), CollectionRole::SmallThumb);
			item->setData (getSize ("m"), CollectionRole::SmallThumbSize);

			AllPhotosItem_->appendRow (item);

			photo = photo.nextSiblingElement ("photo");
		}

		const auto thisPage = photos.attribute ("page").toInt ();
		if (thisPage != photos.attribute ("pages").toInt ())
		{
			auto req = MakeRequest (QString ("http://api.flickr.com/services/rest/"), KQOAuthRequest::AuthorizedRequest);
			req->setAdditionalParameters ({
						{ "method", "flickr.photos.search" },
						{ "user_id", "me" },
						{ "format", "rest" },
						{ "per_page", "500" },
						{ "page", QString::number (thisPage + 1) },
						{ "extras", "url_o,url_z,url_m" }
					});
			AuthMgr_->executeRequest (req);
		}
		else
			emit doneUpdating ();
	}

	void FlickrAccount::checkAuthTokens ()
	{
		if (AuthToken_.isEmpty () || AuthSecret_.isEmpty ())
		{
			qDebug () << Q_FUNC_INFO
					<< "requesting new tokens";
			requestTempToken ();
		}
	}

	void FlickrAccount::requestTempToken ()
	{
		auto req = MakeRequest (RequestTokenURL, KQOAuthRequest::TemporaryCredentials);
		AuthMgr_->executeRequest (req);

		State_ = State::AuthRequested;
	}

	void FlickrAccount::handleTempToken (const QString&, const QString&)
	{
		if (AuthMgr_->lastError () != KQOAuthManager::NoError)
		{
			qWarning () << Q_FUNC_INFO
					<< AuthMgr_->lastError ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq Spegnersi",
						tr ("Unable to get temporary auth token."),
						Priority::Critical));

			QTimer::singleShot (10 * 60 * 1000,
					this,
					SLOT (requestTempToken ()));
			return;
		}

		AuthMgr_->getUserAuthorization (UserAuthURL);
	}

	void FlickrAccount::handleAuthorization (const QString&, const QString&)
	{
		switch (AuthMgr_->lastError ())
		{
		case KQOAuthManager::NoError:
			break;
		case KQOAuthManager::RequestUnauthorized:
			return;
		default:
			qWarning () << Q_FUNC_INFO
					<< AuthMgr_->lastError ();
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Blasq Spegnersi",
						tr ("Unable to get user authorization."),
						Priority::Critical));

			QTimer::singleShot (10 * 60 * 1000,
					this,
					SLOT (requestTempToken ()));
			return;
		}

		AuthMgr_->getUserAccessTokens (AccessTokenURL);
	}

	void FlickrAccount::handleAccessToken (const QString& token, const QString& secret)
	{
		qDebug () << Q_FUNC_INFO
				<< "access token received";
		AuthToken_ = token;
		AuthSecret_ = secret;
		emit accountChanged (this);

		State_ = State::Idle;

		if (UpdateAfterAuth_)
			UpdateCollections ();
	}

	void FlickrAccount::handleReply (const QByteArray& data)
	{
		switch (State_)
		{
		case State::CollectionsRequested:
			HandleCollectionsReply (data);
			break;
		default:
			return;
		}
	}
}
}
}
