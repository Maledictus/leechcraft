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

#include "buddy.h"
#include <QImage>
#include <QtDebug>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/azoth/azothutil.h>
#include "account.h"
#include "util.h"
#include "convimmessage.h"

namespace LeechCraft
{
namespace Azoth
{
namespace VelvetBird
{
	Buddy::Buddy (PurpleBuddy *buddy, Account *account)
	: QObject (account)
	, Account_ (account)
	, Buddy_ (buddy)
	{
		Update ();
	}

	QObject* Buddy::GetQObject ()
	{
		return this;
	}

	IAccount* Buddy::GetParentAccount () const
	{
		return Account_;
	}

	ICLEntry::Features Buddy::GetEntryFeatures () const
	{
		return ICLEntry::FPermanentEntry | ICLEntry::FSupportsGrouping;
	}

	ICLEntry::EntryType Buddy::GetEntryType () const
	{
		return ICLEntry::EntryType::Chat;
	}

	QString Buddy::GetEntryName () const
	{
		return QString::fromUtf8 (purple_buddy_get_alias (Buddy_));
	}

	void Buddy::SetEntryName (const QString& name)
	{
		purple_blist_alias_buddy (Buddy_, name.toUtf8 ().constData ());
	}

	QString Buddy::GetEntryID () const
	{
		return Account_->GetAccountID () + GetHumanReadableID ();
	}

	QString Buddy::GetHumanReadableID () const
	{
		return QString::fromUtf8 (purple_buddy_get_name (Buddy_));
	}

	QStringList Buddy::Groups () const
	{
		return Group_.isEmpty () ? QStringList () : QStringList (Group_);
	}

	void Buddy::SetGroups (const QStringList& groups)
	{
		const auto& newGroup = groups.value (0);

		PurpleGroup *group = 0;
		if (!newGroup.isEmpty ())
		{
			const auto& utf8 = newGroup.toUtf8 ();
			group = purple_find_group (utf8.constData ());
			if (!group)
			{
				group = purple_group_new (utf8.constData ());
				purple_blist_add_group (group, nullptr);
			}
		}

		purple_blist_add_buddy (Buddy_, nullptr, group, nullptr);
	}

	QStringList Buddy::Variants () const
	{
		return QStringList ();
	}

	IMessage* Buddy::CreateMessage (IMessage::Type, const QString&, const QString& body)
	{
		return new ConvIMMessage (body, IMessage::Direction::Out, this);
	}

	QList<IMessage*> Buddy::GetAllMessages () const
	{
		QList<IMessage*> result;
		std::copy (Messages_.begin (), Messages_.end (), std::back_inserter (result));
		return result;
	}

	void Buddy::PurgeMessages (const QDateTime& before)
	{
		AzothUtil::StandardPurgeMessages (Messages_, before);
	}

	void Buddy::SetChatPartState (ChatPartState state, const QString& variant)
	{
	}

	EntryStatus Buddy::GetStatus (const QString& variant) const
	{
		return Status_;
	}

	void Buddy::ShowInfo ()
	{
	}

	QList<QAction*> Buddy::GetActions () const
	{
		return QList<QAction*> ();
	}

	QMap<QString, QVariant> Buddy::GetClientInfo (const QString& variant) const
	{
		return {};
	}

	void Buddy::MarkMsgsRead ()
	{
	}

	void Buddy::ChatTabClosed ()
	{
	}

	void Buddy::Send (ConvIMMessage *msg)
	{
		auto name = purple_buddy_get_name (Buddy_);
		if (!name)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to get buddy name"
					<< Name_;

			const auto& notify = Util::MakeNotification ("Azoth VelvetBird",
					tr ("Unable to send message: protocol error.")
						.arg (name),
					PInfo_);

			const auto& proxy = Account_->GetParentProtocol ()->GetCoreProxy ();
			proxy->GetEntityManager ()->HandleEntity (notify);

			return;
		}

		auto conv = purple_find_conversation_with_account (PURPLE_CONV_TYPE_IM,
				name, Account_->GetPurpleAcc ());
		if (!conv)
		{
			conv = purple_conversation_new (PURPLE_CONV_TYPE_IM, Account_->GetPurpleAcc (), name);
			if (!conv)
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to create conversation with"
						<< name;

				const auto& notify = Util::MakeNotification ("Azoth VelvetBird",
						tr ("Unable to send message to %1: protocol error.")
							.arg (name),
						PInfo_);

				const auto& proxy = Account_->GetParentProtocol ()->GetCoreProxy ();
				proxy->GetEntityManager ()->HandleEntity (notify);
				return;
			}

			conv->ui_data = this;
			purple_conversation_set_logging (conv, false);
		}

		Store (msg);

		purple_conv_im_send (PURPLE_CONV_IM (conv), msg->GetBody ().toUtf8 ().constData ());
	}

	void Buddy::Store (ConvIMMessage *msg)
	{
		Messages_ << msg;
		emit gotMessage (msg);
	}

	void Buddy::SetConv (PurpleConversation *conv)
	{
		conv->ui_data = this;
	}

	void Buddy::HandleMessage (const char *who, const char *body, PurpleMessageFlags flags, time_t time)
	{
		if (flags & PURPLE_MESSAGE_SEND)
			return;

		auto msg = new ConvIMMessage (QString::fromUtf8 (body), IMessage::Direction::In, this);
		if (time)
			msg->SetDateTime (QDateTime::fromTime_t (time));
		Store (msg);
	}

	PurpleBuddy* Buddy::GetPurpleBuddy () const
	{
		return Buddy_;
	}

	void Buddy::Update ()
	{
		if (Name_ != GetEntryName ())
		{
			Name_ = GetEntryName ();
			emit nameChanged (Name_);
		}

		auto purpleStatus = purple_presence_get_active_status (Buddy_->presence);
		const auto& status = FromPurpleStatus (Account_->GetPurpleAcc (), purpleStatus);
		if (status != Status_)
		{
			Status_ = status;
			emit statusChanged (Status_, QString ());
		}

		auto groupNode = purple_buddy_get_group (Buddy_);
		const auto& newGroup = groupNode ? QString::fromUtf8 (groupNode->name) : QString ();
		if (newGroup != Group_)
		{
			Group_ = newGroup;
			emit groupsChanged ({ Group_ });
		}
	}
}
}
}
