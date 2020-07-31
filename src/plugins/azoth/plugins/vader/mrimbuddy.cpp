/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "mrimbuddy.h"
#include <functional>
#include <QImage>
#include <QAction>
#include <QInputDialog>
#include <util/sll/functional.h>
#include <util/xpc/util.h>
#include <util/threads/futures.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/azoth/azothutil.h>
#include <interfaces/azoth/iproxyobject.h>
#include "proto/headers.h"
#include "proto/connection.h"
#include "mrimaccount.h"
#include "mrimmessage.h"
#include "vaderutil.h"
#include "groupmanager.h"
#include "smsdialog.h"
#include "selfavatarfetcher.h"
#include "vcarddialog.h"

namespace LC
{
namespace Azoth
{
namespace Vader
{
	MRIMBuddy::MRIMBuddy (const Proto::ContactInfo& info, MRIMAccount *acc)
	: QObject (acc)
	, A_ (acc)
	, Info_ (info)
	, UpdateNumber_ (new QAction (tr ("Update phone number..."), this))
	, SendSMS_ (new QAction (tr ("Send SMS..."), this))
	, AvatarFetcher_
	{
		new SelfAvatarFetcher
		{
			acc->GetParentProtocol ()->GetCoreProxy ()->GetNetworkAccessManager (),
			info.Email_,
			this
		}
	}
	{
		Status_.State_ = VaderUtil::StatusID2State (info.StatusID_);

		SendSMS_->setProperty ("ActionIcon", "phone");
		connect (UpdateNumber_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleUpdateNumber ()));
		connect (SendSMS_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSendSMS ()));

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { emit avatarChanged (this); },
			AvatarFetcher_,
			SIGNAL (avatarChanged ()),
			this
		};

		UpdateClientVersion ();
	}

	void MRIMBuddy::HandleMessage (MRIMMessage *msg)
	{
		AllMessages_ << msg;
		emit gotMessage (msg);
	}

	void MRIMBuddy::HandleAttention (const QString& msg)
	{
		emit attentionDrawn (msg, {});
	}

	void MRIMBuddy::HandleTune (const QString& tune)
	{
		TuneInfo_ = Media::AudioInfo {};

		const auto& parts = tune.split ('-', Qt::SkipEmptyParts);
		if (parts.size () == 3)
		{
			TuneInfo_.Artist_ = parts.value (0);
			TuneInfo_.Album_ = parts.value (1);
			TuneInfo_.Title_ = parts.value (2);
		}
		else if (parts.size () == 2)
		{
			TuneInfo_.Artist_ = parts.value (0);
			TuneInfo_.Title_ = parts.value (1);
		}
		emit tuneChanged ({});
	}

	void MRIMBuddy::HandleCPS (ChatPartState cps)
	{
		emit chatPartStateChanged (cps, {});
	}

	void MRIMBuddy::SetGroup (const QString& group)
	{
		Info_.GroupNumber_ = A_->GetGroupManager ()->GetGroupNumber (group);
		Group_ = group;
		emit groupsChanged (Groups ());
	}

	void MRIMBuddy::SetAuthorized (bool auth)
	{
		if (auth == IsAuthorized_)
			return;

		IsAuthorized_ = auth;
		if (!IsAuthorized_)
			SetGroup (tr ("Unauthorized"));
		else
			SetGroup ({});
	}

	bool MRIMBuddy::IsAuthorized () const
	{
		return IsAuthorized_;
	}

	void MRIMBuddy::SetGaveSubscription (bool gave)
	{
		GaveSubscription_ = gave;
	}

	bool MRIMBuddy::GaveSubscription () const
	{
		return GaveSubscription_;
	}

	Proto::ContactInfo MRIMBuddy::GetInfo () const
	{
		return Info_;
	}

	namespace
	{
		template<typename T, typename V>
		void CmpXchg (Proto::ContactInfo& info, Proto::ContactInfo newInfo,
				T g, V n)
		{
			if (g (info) != g (newInfo))
			{
				g (info) = g (newInfo);
				n (g (info));
			}
		}

		template<typename T, typename U>
		std::function<T& (Proto::ContactInfo&)> GetMem (U g)
		{
			return g;
		}
	}

	void MRIMBuddy::UpdateInfo (const Proto::ContactInfo& info)
	{
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::Alias_),
				[this] (QString name) { emit nameChanged (name); });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::UA_),
				[this] (QString)
				{
					UpdateClientVersion ();
					emit entryGenerallyChanged ();
				});

		bool stChanged = false;
		const int oldVars = Variants ().size ();
		CmpXchg (Info_, info,
				GetMem<quint32> (&Proto::ContactInfo::StatusID_),
				[&stChanged] (quint32) { stChanged = true; });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::StatusTitle_),
				[&stChanged] (QString) { stChanged = true; });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::StatusDesc_),
				[&stChanged] (QString) { stChanged = true; });

		if (stChanged)
		{
			Status_.State_ = VaderUtil::StatusID2State (Info_.StatusID_);
			Status_.StatusString_ = Info_.StatusTitle_;

			if (oldVars != Variants ().size ())
				emit availableVariantsChanged (Variants ());
			emit statusChanged (GetStatus (QString ()), QString ());
		}

		Info_.GroupNumber_ = info.GroupNumber_;
	}

	void MRIMBuddy::HandleWPInfo (const QMap<QString, QString>& values)
	{
		VCardDialog *dia = new VCardDialog ();
		dia->setAttribute (Qt::WA_DeleteOnClose);
		dia->SetInfo (values);

		const auto am = A_->GetParentProtocol ()->GetAzothProxy ()->GetAvatarsManager ();
		Util::Sequence (dia, am->GetAvatar (this, IHaveAvatars::Size::Full)) >>
				Util::BindMemFn (&VCardDialog::SetAvatar, dia);

		dia->show ();
	}

	qint64 MRIMBuddy::GetID () const
	{
		return Info_.ContactID_;
	}

	void MRIMBuddy::UpdateID (qint64 id)
	{
		Info_.ContactID_ = id;
	}

	QObject* MRIMBuddy::GetQObject ()
	{
		return this;
	}

	MRIMAccount* MRIMBuddy::GetParentAccount () const
	{
		return A_;
	}

	ICLEntry::Features MRIMBuddy::GetEntryFeatures () const
	{
		return FPermanentEntry | FSupportsGrouping | FSupportsRenames;
	}

	ICLEntry::EntryType MRIMBuddy::GetEntryType () const
	{
		return EntryType::Chat;
	}

	QString MRIMBuddy::GetEntryName () const
	{
		return Info_.Alias_.isEmpty () ?
				Info_.Email_ :
				Info_.Alias_;
	}

	void MRIMBuddy::SetEntryName (const QString& name)
	{
		Info_.Alias_ = name;

		A_->GetConnection ()->ModifyContact (GetID (),
				Info_.GroupNumber_, Info_.Email_, name, Info_.Phone_);
		emit nameChanged (name);
	}

	QString MRIMBuddy::GetEntryID () const
	{
		return A_->GetAccountID () + "_" + Info_.Email_;
	}

	QString MRIMBuddy::GetHumanReadableID () const
	{
		return Info_.Email_;
	}

	QStringList MRIMBuddy::Groups () const
	{
		QStringList result;
		if (!Group_.isEmpty ())
			result << Group_;
		return result;
	}

	void MRIMBuddy::SetGroups (const QStringList& list)
	{
		A_->GetGroupManager ()->SetBuddyGroups (this, list);
	}

	QStringList MRIMBuddy::Variants () const
	{
		return Status_.State_ != SOffline ?
				QStringList (QString ()) :
				QStringList ();
	}

	IMessage* MRIMBuddy::CreateMessage (IMessage::Type,
			const QString&, const QString& body)
	{
		const auto msg = new MRIMMessage (IMessage::Direction::Out, IMessage::Type::ChatMessage, this);
		msg->SetBody (body);
		return msg;
	}

	QList<IMessage*> MRIMBuddy::GetAllMessages () const
	{
		QList<IMessage*> result;
		std::copy (AllMessages_.begin (), AllMessages_.end (), std::back_inserter (result));
		return result;
	}

	void MRIMBuddy::PurgeMessages (const QDateTime& before)
	{
		AzothUtil::StandardPurgeMessages (AllMessages_, before);
	}

	void MRIMBuddy::SetChatPartState (ChatPartState state, const QString&)
	{
		A_->SetTypingState (GetHumanReadableID (), state);
	}

	EntryStatus MRIMBuddy::GetStatus (const QString&) const
	{
		return Status_;
	}

	void MRIMBuddy::ShowInfo ()
	{
		A_->RequestInfo (GetHumanReadableID ());
	}

	QList<QAction*> MRIMBuddy::GetActions () const
	{
		return { UpdateNumber_, SendSMS_ };
	}

	QMap<QString, QVariant> MRIMBuddy::GetClientInfo (const QString&) const
	{
		return ClientInfo_;
	}

	void MRIMBuddy::MarkMsgsRead ()
	{
	}

	void MRIMBuddy::ChatTabClosed ()
	{
	}

	Media::AudioInfo MRIMBuddy::GetUserTune (const QString&) const
	{
		return TuneInfo_;
	}

	IAdvancedCLEntry::AdvancedFeatures MRIMBuddy::GetAdvancedFeatures () const
	{
		return AFSupportsAttention;
	}

	void MRIMBuddy::DrawAttention (const QString& text, const QString&)
	{
		A_->GetConnection ()->SendAttention (GetHumanReadableID (), text);
	}

	QFuture<QImage> MRIMBuddy::RefreshAvatar (Size size)
	{
		return AvatarFetcher_->FetchAvatar (size);
	}

	bool MRIMBuddy::HasAvatar () const
	{
		return AvatarFetcher_->IsValid ();
	}

	bool MRIMBuddy::SupportsSize (Size size) const
	{
		switch (size)
		{
		case Size::Full:
		case Size::Thumbnail:
			return true;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown size"
				<< static_cast<int> (size);
		return false;
	}

	void MRIMBuddy::UpdateClientVersion ()
	{
		auto defClient = [this] ()
		{
			ClientInfo_ ["client_type"] = "mailruagent";
			ClientInfo_ ["client_name"] = tr ("Mail.Ru Agent");
			ClientInfo_.remove ("client_version");
		};

		if (Info_.UA_.contains ("leechcraft azoth", Qt::CaseInsensitive))
		{
			ClientInfo_ ["client_type"] = "leechcraft-azoth";
			ClientInfo_ ["client_name"] = "LeechCraft Azoth";

			QString ver = Info_.UA_;
			ver.remove ("leechcraft azoth", Qt::CaseInsensitive);
			ClientInfo_ ["client_version"] = ver.trimmed ();
		}
		else if (Info_.UA_.isEmpty ())
			defClient ();
		else
		{
			qWarning () << Q_FUNC_INFO << "unknown client" << Info_.UA_;

			defClient ();
		}
	}

	void MRIMBuddy::handleUpdateNumber ()
	{
		const auto& num = QInputDialog::getText (0,
				tr ("Update number"),
				tr ("Enter new number in international format:"),
				QLineEdit::Normal,
				Info_.Phone_);
		if (num.isEmpty () || num == Info_.Phone_)
			return;

		Info_.Phone_ = num;
		A_->GetConnection ()->ModifyContact (GetID (),
				Info_.GroupNumber_, Info_.Email_, Info_.Alias_, Info_.Phone_);
	}

	void MRIMBuddy::handleSendSMS ()
	{
		SMSDialog dia (Info_.Phone_);
		if (dia.exec () != QDialog::Accepted)
			return;

		auto conn = A_->GetConnection ();
		const QString& phone = dia.GetPhone ();
		SentSMS_ [conn->SendSMS2Number (phone, dia.GetText ())] = phone;

		connect (conn,
				SIGNAL (smsDelivered (quint32)),
				this,
				SLOT (handleSMSDelivered (quint32)),
				Qt::UniqueConnection);
		connect (conn,
				SIGNAL (smsBadParms (quint32)),
				this,
				SLOT (handleSMSBadParms (quint32)),
				Qt::UniqueConnection);
		connect (conn,
				SIGNAL (smsServiceUnavailable (quint32)),
				this,
				SLOT (handleSMSServUnavail (quint32)),
				Qt::UniqueConnection);
	}

	void MRIMBuddy::handleSMSDelivered (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		const auto iem = A_->GetParentProtocol ()->GetCoreProxy ()->GetEntityManager ();
		iem->HandleEntity (Util::MakeNotification ("Azoth",
					tr ("SMS has been sent to %1.")
						.arg (SentSMS_.take (seq)),
				Priority::Info));
	}

	void MRIMBuddy::handleSMSBadParms (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		const auto iem = A_->GetParentProtocol ()->GetCoreProxy ()->GetEntityManager ();
		iem->HandleEntity (Util::MakeNotification ("Azoth",
					tr ("Failed to send SMS to %1: bad parameters.")
						.arg (SentSMS_.take (seq)),
				Priority::Critical));
	}

	void MRIMBuddy::handleSMSServUnavail (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		const auto iem = A_->GetParentProtocol ()->GetCoreProxy ()->GetEntityManager ();
		iem->HandleEntity (Util::MakeNotification ("Azoth",
					tr ("Failed to send SMS to %1: service unavailable.")
						.arg (SentSMS_.take (seq)),
				Priority::Critical));
	}
}
}
}
