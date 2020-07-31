/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "tracksrestorehandler.h"
#include <QFuture>
#include <QNetworkReply>
#include <util/sll/functional.h>
#include <util/sll/parsejson.h>
#include <util/sll/prelude.h>
#include <util/sll/qtutil.h>
#include <util/sll/queuemanager.h>
#include <util/sll/urlaccessor.h>
#include <util/sll/urloperator.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/svcauth/vkauthmanager.h>
#include <util/threads/futures.h>
#include "util.h"

namespace LC
{
namespace TouchStreams
{
	namespace
	{
		QPair<QString, QString> ParseID (const QString& urlStr)
		{
			const Util::UrlAccessor acc { QUrl::fromEncoded (urlStr.toLatin1 ()) };
			return { acc ["owner_id"], acc ["audio_id"] };
		}

		QHash<QString, QStringList> ToHash (const QList<QPair<QString, QString>>& pairs)
		{
			QHash<QString, QStringList> result;
			for (const auto& pair : pairs)
				result [pair.first] << pair.second;
			return result;
		}
	}

	TracksRestoreHandler::TracksRestoreHandler (const QStringList& ids, QNetworkAccessManager *nam,
			Util::SvcAuth::VkAuthManager *authMgr, Util::QueueManager *queueMgr, QObject *parent)
	: QObject { parent }
	, AuthMgr_ { authMgr }
	, Queue_ { queueMgr }
	, NAM_ { nam }
	, IDs_ { ToHash (Util::Map (ids, ParseID)) }
	{
		FutureIface_.reportStarted ();

		Util::Sequence (this, AuthMgr_->GetAuthKeyFuture ()) >>
				Util::Visitor
				{
					[this] (const QString& token) { Request (token); },
					[] (const Util::SvcAuth::VkAuthManager::AuthKeyError_t&)
					{
						qWarning () << Q_FUNC_INFO
								<< "error getting auth";
					}
				};
	}

	QFuture<Media::RadiosRestoreResult_t> TracksRestoreHandler::GetFuture ()
	{
		return FutureIface_.future ();
	}

	void TracksRestoreHandler::Request (const QString& key)
	{
		for (const auto& pair : Util::Stlize (IDs_))
		{
			const auto& owner = pair.first;
			const auto& audios = pair.second.join (",");

			QUrl url { "https://api.vk.com/method/audio.get" };
			Util::UrlOperator { url }
					("v", "5.37")
					("access_token", key)
					("count", "6000")
					("owner_id", owner)
					("audio_ids", audios);

			const auto reply = NAM_->get (QNetworkRequest { url });
			new Util::SlotClosure<Util::DeleteLaterPolicy>
			{
				[this, reply] { HandleReplyFinished (reply); },
				reply,
				SIGNAL (finished ()),
				reply
			};
		}
	}

	void TracksRestoreHandler::HandleReplyFinished (QNetworkReply *reply)
	{
		const auto notifyGuard = Util::MakeScopeGuard ([this]
				{
					if (!(--PendingRequests_))
						NotifyFuture ();
				});

		const auto& parsedVar = Util::ParseJson (reply, Q_FUNC_INFO);
		reply->deleteLater ();

		const auto& items = parsedVar.toMap () ["response"].toMap () ["items"].toList ();
		for (const auto& item : items)
		{
			const auto& map = item.toMap ();

			const auto& maybeInfo = TrackMap2Info (map);
			if (!maybeInfo)
				continue;

			const auto& radioID = TrackMap2RadioId (map);

			const QList<Media::AudioInfo> infoList { *maybeInfo };
			Result_.append ({ "org.LeechCraft.TouchStreams", radioID, infoList });
		}
	}

	void TracksRestoreHandler::NotifyFuture ()
	{
		FutureIface_.reportFinished (&Result_);
		deleteLater ();
	}
}
}
