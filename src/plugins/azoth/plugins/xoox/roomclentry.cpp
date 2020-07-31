/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "roomclentry.h"
#include <QAction>
#include <QtDebug>
#include <QXmppMucManager.h>
#include <QXmppBookmarkManager.h>
#include <QXmppDiscoveryManager.h>
#include <util/sll/prelude.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/azothutil.h>
#include "clientconnectionextensionsmanager.h"
#include "glooxaccount.h"
#include "glooxprotocol.h"
#include "roompublicmessage.h"
#include "roomhandler.h"
#include "roomconfigwidget.h"
#include "core.h"
#include "util.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	namespace
	{
		auto MakePerms ()
		{
			static const QMap<QByteArray, QList<QByteArray>> map
			{
				{ "role", { "norole", "visitor", "participant", "moderator" } },
				{ "aff", { "outcast", "noaffiliation", "member", "admin", "owner" } }
			};

			return map;
		}

		auto MakeRole2Str ()
		{
			static const QMap<QXmppMucItem::Role, QByteArray> map
			{
				{ QXmppMucItem::NoRole, "norole" },
				{ QXmppMucItem::VisitorRole, "visitor" },
				{ QXmppMucItem::ParticipantRole, "participant" },
				{ QXmppMucItem::ModeratorRole, "moderator" }
			};

			return map;
		}

		auto MakeAff2Str ()
		{
			static const QMap<QXmppMucItem::Affiliation, QByteArray> map
			{
				{ QXmppMucItem::OutcastAffiliation, "outcast" },
				{ QXmppMucItem::NoAffiliation, "noaffiliation" },
				{ QXmppMucItem::MemberAffiliation, "member" },
				{ QXmppMucItem::AdminAffiliation, "admin" },
				{ QXmppMucItem::OwnerAffiliation, "owner" }
			};

			return map;
		}

		auto MakeTranslations ()
		{
			static const QMap<QByteArray, QString> map
			{
				{ "role", RoomCLEntry::tr ("Role") },
				{ "aff", RoomCLEntry::tr ("Affiliation") },
				{ "norole", RoomCLEntry::tr ("Kicked") },
				{ "visitor", RoomCLEntry::tr ("Visitor") },
				{ "participant", RoomCLEntry::tr ("Participant") },
				{ "moderator", RoomCLEntry::tr ("Moderator") },
				{ "outcast", RoomCLEntry::tr ("Banned") },
				{ "noaffiliation", RoomCLEntry::tr ("None") },
				{ "member", RoomCLEntry::tr ("Member") },
				{ "admin", RoomCLEntry::tr ("Admin") },
				{ "owner", RoomCLEntry::tr ("Owner") }
			};

			return map;
		}
	}

	RoomCLEntry::RoomCLEntry (RoomHandler *rh, bool isAutojoined, GlooxAccount *account)
	: QObject (rh)
	, IsAutojoined_ (isAutojoined)
	, Account_ (account)
	, RH_ (rh)
	, Perms_ (MakePerms ())
	, Role2Str_ (MakeRole2Str ())
	, Aff2Str_ (MakeAff2Str ())
	, Translations_ (MakeTranslations ())
	{
		connect (Account_,
				SIGNAL (statusChanged (const EntryStatus&)),
				this,
				SLOT (reemitStatusChange (const EntryStatus&)));

		connect (&Account_->GetClientConnection ()->Exts ().Get<QXmppBookmarkManager> (),
				SIGNAL (bookmarksReceived (QXmppBookmarkSet)),
				this,
				SLOT (handleBookmarks (QXmppBookmarkSet)));
	}

	RoomHandler* RoomCLEntry::GetRoomHandler () const
	{
		return RH_;
	}

	QObject* RoomCLEntry::GetQObject ()
	{
		return this;
	}

	GlooxAccount* RoomCLEntry::GetParentAccount () const
	{
		return Account_;
	}

	ICLEntry::Features RoomCLEntry::GetEntryFeatures () const
	{
		return FSessionEntry;
	}

	ICLEntry::EntryType RoomCLEntry::GetEntryType () const
	{
		return EntryType::MUC;
	}

	QString RoomCLEntry::GetEntryName () const
	{
		const auto& bmManager = Account_->GetClientConnection ()->Exts ().Get<QXmppBookmarkManager> ();
		for (const auto& bm : bmManager.bookmarks ().conferences ())
			if (bm.jid () == RH_->GetRoomJID () && !bm.name ().isEmpty ())
				return bm.name ();

		return RH_->GetRoomJID ();
	}

	void RoomCLEntry::SetEntryName (const QString&)
	{
	}

	QString RoomCLEntry::GetEntryID () const
	{
		return Account_->GetAccountID () + '_' + RH_->GetRoomJID ();
	}

	QString RoomCLEntry::GetHumanReadableID () const
	{
		return RH_->GetRoomJID ();
	}

	QStringList RoomCLEntry::Groups () const
	{
		return { tr ("Multiuser chatrooms") };
	}

	void RoomCLEntry::SetGroups (const QStringList&)
	{
	}

	QStringList RoomCLEntry::Variants () const
	{
		return { "" };
	}

	IMessage* RoomCLEntry::CreateMessage (IMessage::Type,
			const QString& variant, const QString& text)
	{
		if (variant == "")
			return new RoomPublicMessage (text, this);
		else
			return 0;
	}

	QList<IMessage*> RoomCLEntry::GetAllMessages () const
	{
		return AllMessages_;
	}

	void RoomCLEntry::PurgeMessages (const QDateTime& before)
	{
		AzothUtil::StandardPurgeMessages (AllMessages_, before);
	}

	void RoomCLEntry::SetChatPartState (ChatPartState, const QString&)
	{
	}

	EntryStatus RoomCLEntry::GetStatus (const QString&) const
	{
		return Account_->GetState ();
	}

	QList<QAction*> RoomCLEntry::GetActions () const
	{
		QList<QAction*> result;
		RoomParticipantEntry *self = RH_->GetSelf ();
		if (self &&
				self->GetRole () == QXmppMucItem::VisitorRole)
		{
			if (!ActionRequestVoice_)
			{
				ActionRequestVoice_ = new QAction (tr ("Request voice"),
						RH_);
				connect (ActionRequestVoice_,
						SIGNAL (triggered ()),
						RH_,
						SLOT (requestVoice ()));
			}

			result << ActionRequestVoice_;
		}
		return result;
	}

	void RoomCLEntry::ShowInfo ()
	{
	}

	QMap<QString, QVariant> RoomCLEntry::GetClientInfo (const QString&) const
	{
		return QMap<QString, QVariant> ();
	}

	void RoomCLEntry::MarkMsgsRead ()
	{
		Account_->GetParentProtocol ()->GetProxyObject ()->MarkMessagesAsRead (this);
	}

	void RoomCLEntry::ChatTabClosed ()
	{
	}

	IMUCEntry::MUCFeatures RoomCLEntry::GetMUCFeatures () const
	{
		return MUCFCanBeConfigured | MUCFCanInvite;
	}

	QString RoomCLEntry::GetMUCSubject () const
	{
		return RH_->GetSubject ();
	}

	bool RoomCLEntry::CanChangeSubject () const
	{
		return true;
	}

	void RoomCLEntry::SetMUCSubject (const QString& subj)
	{
		RH_->SetSubject (subj);
	}

	QList<QObject*> RoomCLEntry::GetParticipants ()
	{
		return RH_->GetParticipants ();
	}

	bool RoomCLEntry::IsAutojoined () const
	{
		return IsAutojoined_;
	}

	void RoomCLEntry::Join ()
	{
		RH_->Join ();
	}

	void RoomCLEntry::Leave (const QString& msg)
	{
		RH_->Leave (msg);
	}

	QString RoomCLEntry::GetNick () const
	{
		return RH_->GetOurNick ();
	}

	void RoomCLEntry::SetNick (const QString& nick)
	{
		RH_->SetOurNick (nick);
	}

	QString RoomCLEntry::GetGroupName () const
	{
		return tr ("%1 participants").arg (RH_->GetRoomJID ());
	}

	QVariantMap RoomCLEntry::GetIdentifyingData () const
	{
		QVariantMap result;
		const QStringList& list = RH_->GetRoomJID ().split ('@', Qt::SkipEmptyParts);
		const QString& room = list.at (0);
		const QString& server = list.value (1);
		result ["HumanReadableName"] = QString ("%2@%3 (%1)")
				.arg (GetNick ())
				.arg (room)
				.arg (server);
		result ["AccountID"] = Account_->GetAccountID ();
		result ["Nick"] = GetNick ();
		result ["Room"] = room;
		result ["Server"] = server;
		return result;
	}

	QString RoomCLEntry::GetRealID (QObject *obj) const
	{
		RoomParticipantEntry *entry = qobject_cast<RoomParticipantEntry*> (obj);
		if (!entry)
			return QString ();

		return ClientConnection::Split (entry->GetRealJID ()).Bare_;
	}

	void RoomCLEntry::InviteToMUC (const QString& id, const QString& msg)
	{
		RH_->GetRoom ()->sendInvitation (id, msg);
	}

	QMap<QByteArray, QList<QByteArray>> RoomCLEntry::GetPossiblePerms () const
	{
		return Perms_;
	}

	QMap<QByteArray, QList<QByteArray>> RoomCLEntry::GetPerms (QObject *participant) const
	{
		if (!participant)
			participant = RH_->GetSelf ();

		QMap<QByteArray, QList<QByteArray>> result;
		RoomParticipantEntry *entry = qobject_cast<RoomParticipantEntry*> (participant);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< participant
					<< "is not a RoomParticipantEntry";
			result ["role"] << "norole";
			result ["aff"] << "noaffiliation";
		}
		else
		{
			result ["role"] << Role2Str_.value (entry->GetRole (), "invalid");
			result ["aff"] << Aff2Str_.value (entry->GetAffiliation (), "invalid");
		}
		return result;
	}

	QPair<QByteArray, QByteArray> RoomCLEntry::GetKickPerm () const
	{
		return { "role", Role2Str_ [QXmppMucItem::Role::NoRole] };
	}

	QPair<QByteArray, QByteArray> RoomCLEntry::GetBanPerm () const
	{
		return { "aff", Aff2Str_ [QXmppMucItem::Affiliation::OutcastAffiliation] };
	}

	QByteArray RoomCLEntry::GetAffName (QObject *participant) const
	{
		RoomParticipantEntry *entry = qobject_cast<RoomParticipantEntry*> (participant);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< participant
					<< "is not a RoomParticipantEntry";
			return "noaffiliation";
		}

		return Aff2Str_ [entry->GetAffiliation ()];
	}

	namespace
	{
		bool MayChange (QXmppMucItem::Role ourRole,
				QXmppMucItem::Affiliation ourAff,
				RoomParticipantEntry *entry,
				QXmppMucItem::Role newRole)
		{
			QXmppMucItem::Affiliation aff = entry->GetAffiliation ();
			QXmppMucItem::Role role = entry->GetRole ();

			if (role == QXmppMucItem::UnspecifiedRole ||
					ourRole == QXmppMucItem::UnspecifiedRole ||
					newRole == QXmppMucItem::UnspecifiedRole ||
					aff == QXmppMucItem::UnspecifiedAffiliation ||
					ourAff == QXmppMucItem::UnspecifiedAffiliation)
				return false;

			if (ourRole != QXmppMucItem::ModeratorRole)
				return false;

			if (ourAff < aff)
				return false;

			return true;
		}

		bool MayChange (QXmppMucItem::Role,
				QXmppMucItem::Affiliation ourAff,
				RoomParticipantEntry *entry,
				QXmppMucItem::Affiliation aff)
		{
			if (ourAff < QXmppMucItem::AdminAffiliation)
				return false;

			if (ourAff == QXmppMucItem::OwnerAffiliation)
				return true;

			QXmppMucItem::Affiliation partAff = entry->GetAffiliation ();
			if (partAff >= ourAff)
				return false;

			if (aff >= QXmppMucItem::AdminAffiliation)
				return false;

			return true;
		}
	}

	bool RoomCLEntry::MayChangePerm (QObject *participant,
			const QByteArray& permClass, const QByteArray& perm) const
	{
		RoomParticipantEntry *entry = qobject_cast<RoomParticipantEntry*> (participant);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< participant
					<< "is not a RoomParticipantEntry";
			return false;
		}

		const auto ourRole = RH_->GetSelf ()->GetRole ();
		const auto ourAff = RH_->GetSelf ()->GetAffiliation ();

		if (permClass == "role")
			return MayChange (ourRole, ourAff, entry, Role2Str_.key (perm));
		else if (permClass == "aff")
			return MayChange (ourRole, ourAff, entry, Aff2Str_.key (perm));
		else
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown perm class"
					<< permClass;
			return false;
		}
	}

	void RoomCLEntry::SetPerm (QObject *participant,
			const QByteArray& permClass,
			const QByteArray& perm,
			const QString& reason)
	{
		RoomParticipantEntry *entry = qobject_cast<RoomParticipantEntry*> (participant);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< participant
					<< "is not a RoomParticipantEntry";
			return;
		}

		if (permClass == "role")
			RH_->SetRole (entry, Role2Str_.key (perm), reason);
		else if (permClass == "aff")
			RH_->SetAffiliation (entry, Aff2Str_.key (perm), reason);
		else
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown perm class"
					<< permClass;
			return;
		}
	}

	void RoomCLEntry::TrySetPerm (const QString& userId,
			const QByteArray& permClass,
			const QByteArray& targetPerm,
			const QString& reason)
	{
		QXmppMucItem item;
		if (permClass == "role")
			item.setRole (Role2Str_.key (targetPerm));
		else if (permClass == "aff")
			item.setAffiliation (Aff2Str_.key (targetPerm));
		else
			return;

		item.setJid (userId);
		item.setReason (reason);

		Account_->GetClientConnection ()->Update (item, RH_->GetRoomJID ());
	}

	bool RoomCLEntry::IsLessByPerm (QObject *p1, QObject *p2) const
	{
		RoomParticipantEntry *e1 = qobject_cast<RoomParticipantEntry*> (p1);
		RoomParticipantEntry *e2 = qobject_cast<RoomParticipantEntry*> (p2);
		if (!e1 || !e2)
		{
			qWarning () << Q_FUNC_INFO
					<< p1
					<< "or"
					<< p2
					<< "is not a RoomParticipantEntry";
			return false;
		}

		return e1->GetRole () < e2->GetRole ();
	}

	bool RoomCLEntry::IsMultiPerm (const QByteArray&) const
	{
		return false;
	}

	QString RoomCLEntry::GetUserString (const QByteArray& id) const
	{
		return Translations_.value (id, id);
	}

	QWidget* RoomCLEntry::GetConfigurationWidget ()
	{
		return new RoomConfigWidget (this);
	}

	void RoomCLEntry::AcceptConfiguration (QWidget *w)
	{
		RoomConfigWidget *cfg = qobject_cast<RoomConfigWidget*> (w);
		if (!cfg)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to cast"
					<< w
					<< "to RoomConfigWidget";
			return;
		}

		cfg->accept ();
	}

	bool RoomCLEntry::CanSendDirectedStatusNow (const QString&)
	{
		return true;
	}

	void RoomCLEntry::SendDirectedStatus (const EntryStatus& state, const QString&)
	{
		auto conn = Account_->GetClientConnection ();

		auto pres = XooxUtil::StatusToPresence (state.State_,
				state.StatusString_, conn->GetLastState ().Priority_);

		pres.setTo (RH_->GetRoomJID ());

		const auto discoMgr = conn->GetClient ()->findExtension<QXmppDiscoveryManager> ();
		pres.setCapabilityHash ("sha-1");
		pres.setCapabilityNode (discoMgr->clientCapabilitiesNode ());
		pres.setCapabilityVer (discoMgr->capabilities ().verificationString ());
		conn->GetClient ()->sendPacket (pres);
	}

	void RoomCLEntry::MoveMessages (const RoomParticipantEntry_ptr& from, const RoomParticipantEntry_ptr& to)
	{
		for (const auto msgFace : AllMessages_)
		{
			const auto msg = qobject_cast<RoomPublicMessage*> (msgFace->GetQObject ());
			if (msg->OtherPart () == from.get ())
				msg->SetParticipantEntry (to);
		}
	}

	void RoomCLEntry::HandleMessage (RoomPublicMessage *msg)
	{
		Account_->GetParentProtocol ()->GetProxyObject ()->
				GetFormatterProxy ().PreprocessMessage (msg);

		AllMessages_ << msg;
		emit gotMessage (msg);
	}

	void RoomCLEntry::HandleNewParticipants (const QList<ICLEntry*>& parts)
	{
		emit gotNewParticipants (Util::Map (parts, &ICLEntry::GetQObject));
	}

	void RoomCLEntry::HandleSubjectChanged (const QString& subj)
	{
		emit mucSubjectChanged (subj);
	}

	void RoomCLEntry::handleBookmarks (const QXmppBookmarkSet& set)
	{
		for (const auto& bm : set.conferences ())
			if (bm.jid () == RH_->GetRoomJID () && !bm.name ().isEmpty ())
			{
				emit nameChanged (bm.name ());
				break;
			}
	}

	void RoomCLEntry::reemitStatusChange (const EntryStatus& status)
	{
		emit statusChanged (status, QString ());
	}
}
}
}
