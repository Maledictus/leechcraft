/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "accountdatabase.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <util/sll/functor.h>
#include <util/db/dblock.h>
#include <util/db/util.h>
#include <util/db/oral/oral.h>
#include "account.h"
#include "messageinfo.h"
#include "messagebodies.h"

namespace LeechCraft
{
namespace Snails
{
	namespace oral = Util::oral;

	struct AccountDatabase::Message
	{
		oral::PKey<int> Id_;
		oral::Unique<QByteArray> UniqueId_;
		oral::NotNull<bool> IsRead_;
		QString Subject_;
		oral::NotNull<QDateTime> Date_;
		quint64 Size_;
		QByteArray Refs_;
		QByteArray InReplyTos_;

		static QString ClassName ()
		{
			return "Messages";
		}
	};

	struct AccountDatabase::Address
	{
		oral::PKey<int> Id_;
		oral::References<&Message::Id_> MsgId_;
		oral::NotNull<AddressType> AddressType_;
		QString Name_;
		oral::NotNull<QString> Email_;

		static QString ClassName ()
		{
			return "Addresses";
		}
	};

	struct AccountDatabase::Attachment
	{
		oral::PKey<int> Id_;
		oral::References<&Message::Id_> MsgId_;
		QString Name_;
		QString Descr_;
		qint64 Size_;
		QByteArray Type_;
		QByteArray SubType_;

		static QString ClassName ()
		{
			return "Attachments";
		}
	};

	struct AccountDatabase::MessageBodies
	{
		oral::PKey<int> Id_;
		oral::References<&Message::Id_> MsgId_;
		QString PlainText_;
		QString HTML_;

		static QString ClassName ()
		{
			return "MessagesBodies";
		}
	};

	struct AccountDatabase::Folder
	{
		oral::PKey<int> Id_;
		oral::Unique<Util::oral::NotNull<QString>> FolderPath_;

		static QString ClassName ()
		{
			return "Folders";
		}
	};

	struct AccountDatabase::Msg2Folder
	{
		oral::PKey<int> Id_;
		oral::References<&Message::Id_> MsgId_;
		oral::References<&Folder::Id_> FolderId_;
		oral::NotNull<QByteArray> FolderMessageId_;

		static QString ClassName ()
		{
			return "Msg2Folder";
		}
	};

	struct AccountDatabase::MsgHeader
	{
		oral::PKey<int> Id_;
		oral::References<&Message::UniqueId_> MsgUniqueId_;
		oral::NotNull<QByteArray> Header_;

		static QString ClassName ()
		{
			return "MsgHeader";
		}
	};
}
}

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::Message,
		Id_,
		UniqueId_,
		IsRead_,
		Subject_,
		Date_,
		Size_,
		Refs_,
		InReplyTos_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::Address,
		Id_,
		MsgId_,
		AddressType_,
		Name_,
		Email_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::Attachment,
		Id_,
		MsgId_,
		Name_,
		Descr_,
		Size_,
		Type_,
		SubType_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::MessageBodies,
		Id_,
		MsgId_,
		PlainText_,
		HTML_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::Folder,
		Id_,
		FolderPath_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::Msg2Folder,
		Id_,
		MsgId_,
		FolderId_,
		FolderMessageId_)

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Snails::AccountDatabase::MsgHeader,
		Id_,
		MsgUniqueId_,
		Header_)

namespace LeechCraft
{
namespace Snails
{
	AccountDatabase::AccountDatabase (const QDir& dir, const Account *acc)
	: DB_ { QSqlDatabase::addDatabase ("QSQLITE", "SnailsStorage_" + acc->GetID ()) }
	{
		DB_.setDatabaseName (dir.filePath ("msgs.db"));
		if (!DB_.open ())
		{
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error (qPrintable (QString ("Could not initialize database: %1")
						.arg (DB_.lastError ().text ())));
		}

		Util::RunTextQuery (DB_, "PRAGMA foreign_keys = ON;");
		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		Messages_ = Util::oral::AdaptPtr<Message> (DB_);
		Addresses_ = Util::oral::AdaptPtr<Address> (DB_);
		Attachments_ = Util::oral::AdaptPtr<Attachment> (DB_);

		MessagesBodies_ = Util::oral::AdaptPtr<MessageBodies> (DB_);

		Folders_ = Util::oral::AdaptPtr<Folder> (DB_);
		Msg2Folder_ = Util::oral::AdaptPtr<Msg2Folder> (DB_);
		MsgHeader_ = Util::oral::AdaptPtr<MsgHeader> (DB_);

		LoadKnownFolders ();
	}

	Util::DBLock AccountDatabase::BeginTransaction ()
	{
		Util::DBLock lock { DB_ };
		lock.Init ();
		return lock;
	}

	namespace sph = oral::sph;

	namespace
	{
		struct WithMessagesType {};
		struct WithoutMessagesType {};

		constexpr WithMessagesType WithMessages {};
		constexpr WithoutMessagesType WithoutMessages {};

		template<typename MessageTableSelector>
		auto FolderMessageIdSelector (const QByteArray& msgId, const QStringList& folder, MessageTableSelector)
		{
			using A = AccountDatabase;

			auto common = sph::f<&A::Folder::FolderPath_> == folder.join ("/") &&
					sph::f<&A::Msg2Folder::FolderMessageId_> == msgId &&
					sph::f<&A::Folder::Id_> == sph::f<&A::Msg2Folder::FolderId_>;

			if constexpr (std::is_same_v<MessageTableSelector, WithMessagesType>)
				return common &&
						sph::f<&A::Message::Id_> == sph::f<&A::Msg2Folder::MsgId_>;
			else
				return common;
		}
	}

	QList<QByteArray> AccountDatabase::GetIDs (const QStringList& folder)
	{
		return Msg2Folder_->Select (sph::fields<&Msg2Folder::FolderMessageId_>,
				sph::f<&Folder::FolderPath_> == folder.join ("/") &&
				sph::f<&Folder::Id_> == sph::f<&Msg2Folder::FolderId_>);
	}

	std::optional<QByteArray> AccountDatabase::GetLastID (const QStringList& folder)
	{
		using Util::operator*;
		return [] (auto tup) { return std::get<0> (tup); } *
			Msg2Folder_->SelectOne.Build ()
				.Select (sph::fields<&Msg2Folder::FolderMessageId_, &Msg2Folder::Id_>)
				.Where (sph::f<&Folder::FolderPath_> == folder.join ("/") &&
						sph::f<&Folder::Id_> == sph::f<&Msg2Folder::FolderId_>)
				.Order (oral::OrderBy<sph::desc<&Msg2Folder::Id_>>)
				.Limit (1)
				();
	}

	int AccountDatabase::GetMessageCount (const QStringList& folder)
	{
		return Msg2Folder_->Select (sph::count<>,
				sph::f<&Folder::FolderPath_> == folder.join ("/") &&
				sph::f<&Folder::Id_> == sph::f<&Msg2Folder::FolderId_>);
	}

	int AccountDatabase::GetUnreadMessageCount (const QStringList& folder)
	{
		return Msg2Folder_->Select (sph::count<>,
				sph::f<&Folder::FolderPath_> == folder.join ("/") &&
				sph::f<&Folder::Id_> == sph::f<&Msg2Folder::FolderId_> &&
				sph::f<&Message::Id_> == sph::f<&Msg2Folder::MsgId_> &&
				sph::f<&Message::IsRead_> == false);
	}

	int AccountDatabase::GetMessageCount ()
	{
		return Messages_->Select (sph::count<>);
	}

	std::optional<int> AccountDatabase::GetMsgTableId (const QByteArray& uniqueId)
	{
		if (uniqueId.isEmpty ())
			return {};

		return Messages_->SelectOne (sph::fields<&Message::Id_>,
				sph::f<&Message::UniqueId_> == uniqueId);
	}

	std::optional<int> AccountDatabase::GetMsgTableId (const QByteArray& msgId, const QStringList& folder)
	{
		return Msg2Folder_->SelectOne (sph::fields<&Msg2Folder::MsgId_>,
				FolderMessageIdSelector (msgId, folder, WithoutMessages));
	}

	namespace
	{
		MessageInfo MakeMessageInfo (const AccountDatabase::Message& item,
				const QStringList& folder, const QByteArray& msgId)
		{
			return
			{
				item.IsRead_,
				item.UniqueId_,
				msgId,
				folder,
				item.Subject_,
				item.Date_,
				item.Size_,
				{},
				item.Refs_.split ('\n'),
				item.InReplyTos_.split ('\n'),
				{}
			};
		}

		void AddAddress (MessageInfo& info, const AccountDatabase::Address& addr)
		{
			info.Addresses_ [addr.AddressType_].push_back ({ addr.Name_, addr.Email_ });
		}

		void AddAttachment (MessageInfo& info, const AccountDatabase::Attachment& att)
		{
			info.Attachments_.push_back ({
					att.Name_,
					att.Descr_,
					att.Type_,
					att.SubType_,
					att.Size_
				});
		}
	}

	QList<MessageInfo> AccountDatabase::GetMessageInfos (const QStringList& folder)
	{
		QHash<int, MessageInfo> result;

		const auto byFolderQuery = sph::f<&Folder::FolderPath_> == folder.join ("/") &&
					sph::f<&Folder::Id_> == sph::f<&Msg2Folder::FolderId_> &&
					sph::f<&Message::Id_> == sph::f<&Msg2Folder::MsgId_>;

		{
			auto res = Messages_->Select (sph::all + sph::fields<&Msg2Folder::FolderMessageId_>,
					byFolderQuery);
			result.reserve (res.size ());
			for (const auto& [item, msgId] : res)
				result [item.Id_] = MakeMessageInfo (item, folder, msgId);
		}

		{
			auto res = Addresses_->Select (sph::all,
					sph::f<&Address::MsgId_> == sph::f<&Message::Id_> &&
					byFolderQuery);
			for (const auto& addr : res)
				AddAddress (result [addr.MsgId_], addr);
		}

		{
			auto res = Attachments_->Select (sph::all,
					sph::f<&Attachment::MsgId_> == sph::f<&Message::Id_> &&
					byFolderQuery);
			for (const auto& att : res)
				AddAttachment (result [att.MsgId_], att);
		}

		return result.values ();
	}

	void AccountDatabase::AddMessage (const MessageInfo& msg)
	{
		const auto& folder = msg.Folder_;
		AddFolder (folder);

		if (GetMsgTableId (msg.FolderId_, folder))
		{
			qWarning () << Q_FUNC_INFO
					<< "skipping existing message"
					<< msg.FolderId_
					<< "in folder"
					<< folder;
			return;
		}

		Util::DBLock lock { DB_ };
		lock.Init ();

		const auto existing = GetMsgTableId (msg.MessageId_);
		const auto msgTableId = existing ?
				*existing :
				AddMessageUnfoldered (msg);
		AddMessageToFolder (msgTableId, GetFolder (folder), msg.FolderId_);

		lock.Good ();
	}

	void AccountDatabase::RemoveMessage (const QByteArray& msgId, const QStringList& folder)
	{
		const auto id = Msg2Folder_->SelectOne (sph::fields<&Msg2Folder::Id_>,
				FolderMessageIdSelector (msgId, folder, WithoutMessages));
		if (id)
			Msg2Folder_->DeleteBy (sph::f<&Msg2Folder::Id_> == *id);
	}

	void AccountDatabase::SaveMessageBodies (const QStringList& folder,
			const QByteArray& msgId, const Snails::MessageBodies& bodies)
	{
		auto msgPKey = GetMsgTableId (msgId, folder);
		if (!msgPKey)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown message"
					<< folder
					<< msgId;
			return;
		}

		MessagesBodies_->Insert ({ {}, *msgPKey, bodies.PlainText_, bodies.HTML_ });
	}

	std::optional<MessageBodies> AccountDatabase::GetMessageBodies (const QStringList& folder, const QByteArray& msgId)
	{
		auto msgPKey = GetMsgTableId (msgId, folder);
		if (!msgPKey)
			return {};

		using Util::operator*;

		return MessagesBodies_->SelectOne (sph::fields<&MessageBodies::PlainText_, &MessageBodies::HTML_>,
				sph::f<&MessageBodies::MsgId_> == *msgPKey) *
				[] (auto&& tup) { return Snails::MessageBodies { std::get<0> (tup), std::get<1> (tup) }; };
	}

	std::optional<bool> AccountDatabase::IsMessageRead (const QByteArray& msgId, const QStringList& folder)
	{
		return Messages_->SelectOne (sph::fields<&Message::IsRead_>,
				FolderMessageIdSelector (msgId, folder, WithMessages));
	}

	void AccountDatabase::SetMessageRead (const QByteArray& msgId, const QStringList& folder, bool read)
	{
		auto msgTableId = GetMsgTableId (msgId, folder);
		if (!msgTableId)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown message"
					<< msgId
					<< "in folder"
					<< folder;
			return;
		}
		Messages_->Update (sph::f<&Message::IsRead_> = read,
				sph::f<&Message::Id_> == *msgTableId);
	}

	void AccountDatabase::SetMessageHeader (const QByteArray& msgId, const QByteArray& header)
	{
		MsgHeader_->Insert ({ {}, msgId, header }, oral::InsertAction::Replace::PKey<MsgHeader>);
	}

	std::optional<QByteArray> AccountDatabase::GetMessageHeader (const QByteArray& uniqueMsgId) const
	{
		return MsgHeader_->SelectOne (sph::fields<&MsgHeader::Header_>,
				sph::f<&MsgHeader::MsgUniqueId_> == uniqueMsgId);
	}

	std::optional<QByteArray> AccountDatabase::GetMessageHeader (const QStringList& folderId, const QByteArray& msgId) const
	{
		return MsgHeader_->SelectOne (sph::fields<&MsgHeader::Header_>,
		        FolderMessageIdSelector (msgId, folderId, WithMessages) &&
		        sph::f<&MsgHeader::MsgUniqueId_> == sph::f<&Message::UniqueId_>);
	}

	namespace
	{
		template<char Ch>
		auto Join (const QList<QByteArray>& arrs)
		{
			if (arrs.isEmpty ())
				return QByteArray {};

			QByteArray result;
			const auto totalLength = std::accumulate (arrs.begin (), arrs.end (), 0,
					[] (int acc, const auto& arr) { return acc + arr.size (); });
			result.reserve (arrs.size () - 1 + totalLength);
			bool isFirst = true;
			for (const auto& arr : arrs)
			{
				if (!isFirst)
					result += Ch;
				isFirst = false;
				result += arr;
			}
			return result;
		}
	}

	int AccountDatabase::AddMessageUnfoldered (const MessageInfo& msg)
	{
		auto id = Messages_->Insert ({
				{},
				msg.MessageId_,
				msg.IsRead_,
				msg.Subject_,
				msg.Date_,
				msg.Size_,
				Join<'\n'> (msg.References_),
				Join<'\n'> (msg.InReplyTo_)
			});

		for (const auto& [type, addrs] : Util::Stlize (msg.Addresses_))
			for (const auto& addr : addrs)
				Addresses_->Insert ({ {}, id, type, addr.Name_, addr.Email_ });

		for (const auto& att : msg.Attachments_)
			Attachments_->Insert ({
					{},
					id,
					att.GetName (),
					att.GetDescr (),
					att.GetSize (),
					att.GetType (),
					att.GetSubType ()
				});

		return id;
	}

	void AccountDatabase::AddMessageToFolder (int msgTableId, int folderTableId, const QByteArray& msgId)
	{
		Msg2Folder_->Insert ({ {}, msgTableId, folderTableId, msgId });
	}

	int AccountDatabase::AddFolder (const QStringList& folder)
	{
		if (KnownFolders_.contains (folder))
			return KnownFolders_.value (folder);

		const auto id = Folders_->Insert ({ {}, { folder.join ("/") } });
		KnownFolders_ [folder] = id;
		return id;
	}

	int AccountDatabase::GetFolder (const QStringList& folder) const
	{
		if (!KnownFolders_.contains (folder))
			throw std::runtime_error ("Unknown folder");

		return KnownFolders_.value (folder);
	}

	void AccountDatabase::LoadKnownFolders ()
	{
		for (const auto& folder : Folders_->Select ())
			KnownFolders_ [(**folder.FolderPath_).split ('/')] = folder.Id_;
	}
}
}
