/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "resourcesfetcher.h"
#include <interfaces/core/ientitymanager.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/sys/paths.h>
#include <util/threads/futures.h>
#include <util/xpc/util.h>
#include "storagebackendmanager.h"

namespace LC::Aggregator
{
	ResourcesFetcher::ResourcesFetcher (IEntityManager *iem, QObject *parent)
	: QObject { parent }
	, EntityManager_ { iem }
	{
		connect (&StorageBackendManager::Instance (),
				&StorageBackendManager::channelAdded,
				this,
				[this] (const Channel& channel)
				{
					FetchPixmap (channel.ChannelID_, channel.PixmapURL_);
					FetchFavicon (channel.ChannelID_, channel.Link_);
				});
	}

	void ResourcesFetcher::FetchPixmap (IDType_t cid, const QString& pixmapUrl)
	{
		auto sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();

		QUrl urlObj { pixmapUrl };
		if (urlObj.isValid () && !urlObj.isRelative ())
			FetchExternalFile (pixmapUrl,
					[sb, cid] (const QString& path) { sb->SetChannelPixmap (cid, QImage { path }); });
	}

	void ResourcesFetcher::FetchFavicon (IDType_t cid, const QString& link)
	{
		QUrl oldUrl { link };
		oldUrl.setPath ("/favicon.ico");
		QString iconUrl = oldUrl.toString ();

		auto sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();

		FetchExternalFile (iconUrl,
				[sb, cid] (const QString& path) { sb->SetChannelFavicon (cid, QImage { path }); });
	}

	void ResourcesFetcher::FetchExternalFile (const QString& url, const std::function<void (QString)>& cont)
	{
		auto where = Util::GetTemporaryName ();

		const auto& e = Util::MakeEntity (QUrl (url),
				where,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto& delegateResult = EntityManager_->DelegateEntity (e);
		if (!delegateResult)
		{
			qWarning () << Q_FUNC_INFO
					<< "no plugin to delegate"
					<< url;
			return;
		}

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success) { cont (where); },
					[] (const IDownload::Error&) {}
				}.Finally ([where] { QFile::remove (where); });
	}

}
