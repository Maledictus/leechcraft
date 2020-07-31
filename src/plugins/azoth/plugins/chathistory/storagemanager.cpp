/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "storagemanager.h"
#include <cmath>
#include <QMessageBox>
#include <util/util.h>
#include <util/threads/futures.h>
#include <util/threads/workerthreadbase.h>
#include <util/sll/visitor.h>
#include <util/db/consistencychecker.h>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/irichtextmessage.h>
#include "storage.h"
#include "loggingstatekeeper.h"

namespace LC
{
namespace Azoth
{
namespace ChatHistory
{
	StorageManager::StorageManager (LoggingStateKeeper *keeper)
	: StorageThread_ { std::make_shared<StorageThread> () }
	, LoggingStateKeeper_ { keeper }
	{
		StorageThread_->SetPaused (true);
		StorageThread_->SetAutoQuit (true);

		Util::Sequence (this, StorageThread_->ScheduleImpl (&Storage::Initialize)) >>
				[this] (const Storage::InitializationResult_t& res)
				{
					if (res.IsRight ())
					{
						StorageThread_->SetPaused (false);
						return;
					}

					HandleStorageError (res.GetLeft ());
				};

		auto checker = Util::ConsistencyChecker::Create (Storage::GetDatabasePath (), "Azoth ChatHistory");
		Util::Sequence (this, checker->StartCheck ()) >>
				Util::Visitor
				{
					[this] (const Util::ConsistencyChecker::Succeeded&) { StartStorage (); },
					[this] (const Util::ConsistencyChecker::Failed& failed)
					{
						qWarning () << Q_FUNC_INFO
								<< "db is broken, gonna repair";
						Util::Sequence (this, failed->DumpReinit ()) >>
								Util::Visitor
								{
									[] (const Util::ConsistencyChecker::DumpError& err)
									{
										QMessageBox::critical (nullptr,
												"Azoth ChatHistory",
												err.Error_);
									},
									[this] (const Util::ConsistencyChecker::DumpFinished& res)
									{
										HandleDumpFinished (res.OldFileSize_, res.NewFileSize_);
									}
								};
					}
				};
	}

	namespace
	{
		QString GetVisibleName (const ICLEntry *entry)
		{
			if (entry->GetEntryType () == ICLEntry::EntryType::PrivateChat)
			{
				const auto parent = entry->GetParentCLEntry ();
				return parent->GetEntryName () + "/" + entry->GetEntryName ();
			}
			else
				return entry->GetEntryName ();
		}
	}

	void StorageManager::Process (QObject *msgObj)
	{
		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (msg->GetMessageType () != IMessage::Type::ChatMessage &&
			msg->GetMessageType () != IMessage::Type::MUCMessage)
			return;

		if (msg->GetBody ().isEmpty ())
			return;

		if (msg->GetDirection () == IMessage::Direction::Out &&
				msg->GetMessageType () == IMessage::Type::MUCMessage)
			return;

		const double secsDiff = msg->GetDateTime ().secsTo (QDateTime::currentDateTime ());
		if (msg->GetMessageType () == IMessage::Type::MUCMessage &&
				std::abs (secsDiff) >= 2)
			return;

		ICLEntry *entry = qobject_cast<ICLEntry*> (msg->ParentCLEntry ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "message's other part doesn't implement ICLEntry"
					<< msg->GetQObject ()
					<< msg->OtherPart ();
			return;
		}
		if (!LoggingStateKeeper_->IsLoggingEnabled (entry))
			return;

		const auto irtm = qobject_cast<IRichTextMessage*> (msgObj);

		AddLogItems (entry->GetParentAccount ()->GetAccountID (),
				entry->GetEntryID (),
				GetVisibleName (entry),
				{
					LogItem
					{
						msg->GetDateTime (),
						msg->GetDirection (),
						msg->GetBody (),
						msg->GetOtherVariant (),
						msg->GetMessageType (),
						irtm ? irtm->GetRichBody () : QString {},
						msg->GetEscapePolicy ()
					}
				},
				false);
	}

	void StorageManager::AddLogItems (const QString& accountId, const QString& entryId,
			const QString& visibleName, const QList<LogItem>& items, bool fuzzy)
	{
		StorageThread_->ScheduleImpl (&Storage::AddMessages,
				accountId,
				entryId,
				visibleName,
				items,
				fuzzy);
	}

	QFuture<IHistoryPlugin::MaxTimestampResult_t> StorageManager::GetMaxTimestamp (const QString& accId)
	{
		return StorageThread_->ScheduleImpl (&Storage::GetMaxTimestamp, accId);
	}

	QFuture<QStringList> StorageManager::GetOurAccounts ()
	{
		return StorageThread_->ScheduleImpl (&Storage::GetOurAccounts);
	}

	QFuture<UsersForAccountResult_t> StorageManager::GetUsersForAccount (const QString& accountID)
	{
		return StorageThread_->ScheduleImpl (&Storage::GetUsersForAccount, accountID);
	}

	QFuture<ChatLogsResult_t> StorageManager::GetChatLogs (const QString& accountId,
			const QString& entryId, int backpages, int amount)
	{
		return StorageThread_->ScheduleImpl (&Storage::GetChatLogs, accountId, entryId, backpages, amount);
	}

	QFuture<SearchResult_t> StorageManager::Search (const QString& accountId, const QString& entryId,
			const QString& text, int shift, bool cs)
	{
		return StorageThread_->ScheduleImpl (&Storage::Search, accountId, entryId, text, shift, cs);
	}

	QFuture<SearchResult_t> StorageManager::Search (const QString& accountId, const QString& entryId, const QDateTime& dt)
	{
		return StorageThread_->ScheduleImpl (&Storage::SearchDate, accountId, entryId, dt);
	}

	QFuture<DaysResult_t> StorageManager::GetDaysForSheet (const QString& accountId, const QString& entryId, int year, int month)
	{
		return StorageThread_->ScheduleImpl (&Storage::GetDaysForSheet, accountId, entryId, year, month);
	}

	void StorageManager::ClearHistory (const QString& accountId, const QString& entryId)
	{
		StorageThread_->ScheduleImpl (&Storage::ClearHistory, accountId, entryId);
	}

	void StorageManager::RegenUsersCache ()
	{
		StorageThread_->ScheduleImpl (&Storage::RegenUsersCache);
	}

	void StorageManager::StartStorage ()
	{
		StorageThread_->SetPaused (false);
		StorageThread_->start (QThread::LowestPriority);
	}

	void StorageManager::HandleStorageError (const Storage::InitializationError_t& error)
	{
		Util::Visit (error,
				[] (const Storage::GeneralError& err)
				{
					QMessageBox::critical (nullptr,
							"Azoth ChatHistory",
							tr ("Unable to initialize permanent storage. %1.")
								.arg (err.ErrorText_));
				});
	}

	void StorageManager::HandleDumpFinished (qint64 oldSize, qint64 newSize)
	{
		StartStorage ();

		Util::Sequence (this, StorageThread_->ScheduleImpl (&Storage::GetAllHistoryCount)) >>
				[=] (const boost::optional<int>& count)
				{
					const auto& text = QObject::tr ("Finished restoring history database contents. "
							"Old file size: %1, new file size: %2, %3 records recovered.");
					const auto& greet = newSize > oldSize * 0.9 ?
							QObject::tr ("Yay, seems like most of the contents are intact!") :
							QObject::tr ("Sadly, seems like quite some history is lost.");

					QMessageBox::information (nullptr,
							"Azoth ChatHistory",
							text.arg (Util::MakePrettySize (oldSize))
								.arg (Util::MakePrettySize (newSize))
								.arg (count.get_value_or (0)) +
								" " + greet);
				};
	}
}
}
}
