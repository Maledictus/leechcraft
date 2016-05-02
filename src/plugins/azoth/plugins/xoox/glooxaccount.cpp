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

#include "glooxaccount.h"
#include <memory>
#include <QInputDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QXmppCallManager.h>
#include <QXmppMucManager.h>
#include <util/xpc/util.h>
#include <util/sll/prelude.h>
#include <util/sll/slotclosure.h>
#include <util/sll/util.h>
#include <util/sll/either.h>
#include <util/sll/void.h>
#include <interfaces/azoth/iprotocol.h>
#include <interfaces/azoth/iproxyobject.h>

#ifdef ENABLE_MEDIACALLS
#include "mediacall.h"
#endif
#ifdef ENABLE_CRYPT
#include "pgpmanager.h"
#endif

#include "glooxprotocol.h"
#include "core.h"
#include "clientconnection.h"
#include "glooxmessage.h"
#include "glooxclentry.h"
#include "roomclentry.h"
#include "transfermanager.h"
#include "sdsession.h"
#include "pubsubmanager.h"
#include "usertune.h"
#include "usermood.h"
#include "useractivity.h"
#include "userlocation.h"
#include "privacylistsconfigdialog.h"
#include "pepmicroblog.h"
#include "jabbersearchsession.h"
#include "bookmarkeditwidget.h"
#include "accountsettingsholder.h"
#include "crypthandler.h"
#include "gwitemsremovaldialog.h"
#include "serverinfostorage.h"
#include "xep0313manager.h"
#include "xep0313prefsdialog.h"
#include "xep0313modelmanager.h"
#include "pendinglastactivityrequest.h"
#include "lastactivitymanager.h"
#include "roomhandler.h"
#include "clientconnectionerrormgr.h"
#include "addtoblockedrunner.h"
#include "util.h"
#include "selfcontact.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	bool operator== (const GlooxAccountState& s1, const GlooxAccountState& s2)
	{
		return s1.Priority_ == s2.Priority_ &&
			s1.State_ == s2.State_ &&
			s1.Status_ == s2.Status_;
	}

	GlooxAccount::GlooxAccount (const QString& name,
			GlooxProtocol *proto,
			QObject *parent)
	: QObject (parent)
	, Name_ (name)
	, ParentProtocol_ (proto)
	, SettingsHolder_ (new AccountSettingsHolder (this))
	, SelfVCardAction_ (new QAction (tr ("Self VCard..."), this))
	, PrivacyDialogAction_ (new QAction (tr ("Privacy lists..."), this))
	, CarbonsAction_ (new QAction (tr ("Enable message carbons"), this))
	, Xep0313ModelMgr_ (new Xep0313ModelManager (this))
	{
		SelfVCardAction_->setProperty ("ActionIcon", "text-x-vcard");
		PrivacyDialogAction_->setProperty ("ActionIcon", "emblem-locked");
		CarbonsAction_->setProperty ("ActionIcon", "edit-copy");

		CarbonsAction_->setCheckable (true);
		CarbonsAction_->setToolTip (tr ("Deliver messages from conversations on "
					"other resources to this resource as well."));

		connect (SelfVCardAction_,
				SIGNAL (triggered ()),
				this,
				SLOT (showSelfVCard ()));
		connect (PrivacyDialogAction_,
				SIGNAL (triggered ()),
				this,
				SLOT (showPrivacyDialog ()));
		connect (CarbonsAction_,
				SIGNAL (toggled (bool)),
				this,
				SLOT (handleCarbonsToggled (bool)));

		connect (SettingsHolder_,
				SIGNAL (accountSettingsChanged ()),
				this,
				SIGNAL (accountSettingsChanged ()));
		connect (SettingsHolder_,
				SIGNAL (jidChanged (QString)),
				this,
				SLOT (regenAccountIcon (QString)));
	}

	void GlooxAccount::Init ()
	{
		ClientConnection_.reset (new ClientConnection (this));

		TransferManager_.reset (new TransferManager (ClientConnection_->GetTransferManager (),
					this));

		connect (ClientConnection_.get (),
				SIGNAL (gotConsoleLog (QByteArray, IHaveConsole::PacketDirection, QString)),
				this,
				SIGNAL (gotConsolePacket (QByteArray, IHaveConsole::PacketDirection, QString)));

		connect (ClientConnection_.get (),
				SIGNAL (serverAuthFailed ()),
				this,
				SLOT (handleServerAuthFailed ()));
		connect (ClientConnection_.get (),
				SIGNAL (needPassword ()),
				this,
				SLOT (feedClientPassword ()));

		connect (ClientConnection_.get (),
				SIGNAL (statusChanged (const EntryStatus&)),
				this,
				SIGNAL (statusChanged (const EntryStatus&)));

		connect (ClientConnection_.get (),
				SIGNAL (gotRosterItems (const QList<QObject*>&)),
				this,
				SLOT (handleGotRosterItems (const QList<QObject*>&)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemRemoved (QObject*)),
				this,
				SLOT (handleEntryRemoved (QObject*)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemsRemoved (const QList<QObject*>&)),
				this,
				SIGNAL (removedCLItems (const QList<QObject*>&)));
		connect (ClientConnection_.get (),
				SIGNAL (gotSubscriptionRequest (QObject*, const QString&)),
				this,
				SIGNAL (authorizationRequested (QObject*, const QString&)));

		connect (ClientConnection_.get (),
				SIGNAL (rosterItemSubscribed (QObject*, const QString&)),
				this,
				SIGNAL (itemSubscribed (QObject*, const QString&)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemUnsubscribed (QObject*, const QString&)),
				this,
				SIGNAL (itemUnsubscribed (QObject*, const QString&)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemUnsubscribed (const QString&, const QString&)),
				this,
				SIGNAL (itemUnsubscribed (const QString&, const QString&)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemCancelledSubscription (QObject*, const QString&)),
				this,
				SIGNAL (itemCancelledSubscription (QObject*, const QString&)));
		connect (ClientConnection_.get (),
				SIGNAL (rosterItemGrantedSubscription (QObject*, const QString&)),
				this,
				SIGNAL (itemGrantedSubscription (QObject*, const QString&)));
		connect (ClientConnection_.get (),
				SIGNAL (gotMUCInvitation (QVariantMap, QString, QString)),
				this,
				SIGNAL (mucInvitationReceived (QVariantMap, QString, QString)));

		connect (ClientConnection_->GetXep0313Manager (),
				SIGNAL (serverHistoryFetched (QString, QString, SrvHistMessages_t)),
				this,
				SLOT (handleServerHistoryFetched (QString, QString, SrvHistMessages_t)));

#ifdef ENABLE_MEDIACALLS
		connect (ClientConnection_->GetCallManager (),
				SIGNAL (callReceived (QXmppCall*)),
				this,
				SLOT (handleIncomingCall (QXmppCall*)));
#endif

		regenAccountIcon (SettingsHolder_->GetJID ());

		CarbonsAction_->setChecked (SettingsHolder_->IsMessageCarbonsEnabled ());
	}

	void GlooxAccount::Release ()
	{
		emit removedCLItems (GetCLEntries ());
	}

	AccountSettingsHolder* GlooxAccount::GetSettings () const
	{
		return SettingsHolder_;
	}

	QObject* GlooxAccount::GetQObject ()
	{
		return this;
	}

	GlooxProtocol* GlooxAccount::GetParentProtocol () const
	{
		return ParentProtocol_;
	}

	IAccount::AccountFeatures GlooxAccount::GetAccountFeatures () const
	{
		return FRenamable | FSupportsXA | FMUCsSupportFileTransfers | FCanViewContactsInfoInOffline;
	}

	QList<QObject*> GlooxAccount::GetCLEntries ()
	{
		return ClientConnection_ ?
				ClientConnection_->GetCLEntries () :
				QList<QObject*> ();
	}

	QString GlooxAccount::GetAccountName () const
	{
		return Name_;
	}

	QString GlooxAccount::GetOurNick () const
	{
		return SettingsHolder_->GetNick ();
	}

	void GlooxAccount::RenameAccount (const QString& name)
	{
		Name_ = name;
		emit accountRenamed (name);
		emit accountSettingsChanged ();
	}

	QByteArray GlooxAccount::GetAccountID () const
	{
		return ParentProtocol_->GetProtocolID () + "_" + SettingsHolder_->GetJID ().toUtf8 ();
	}

	QList<QAction*> GlooxAccount::GetActions () const
	{
		return { SelfVCardAction_, PrivacyDialogAction_, CarbonsAction_ };
	}

	void GlooxAccount::OpenConfigurationDialog ()
	{
		SettingsHolder_->OpenConfigDialog ();
	}

	EntryStatus GlooxAccount::GetState () const
	{
		const auto& state = ClientConnection_ ?
				ClientConnection_->GetLastState () :
				GlooxAccountState ();
		return EntryStatus (state.State_, state.Status_);
	}

	void GlooxAccount::ChangeState (const EntryStatus& status)
	{
		if (status.State_ == SOffline &&
				!ClientConnection_)
			return;

		if (!ClientConnection_)
			Init ();

		auto state = ClientConnection_->GetLastState ();
		state.State_ = status.State_;
		state.Status_ = status.StatusString_;
		ClientConnection_->SetState (state);
	}

	void GlooxAccount::Authorize (QObject *entryObj)
	{
		ClientConnection_->AckAuth (entryObj, true);
	}

	void GlooxAccount::DenyAuth (QObject *entryObj)
	{
		ClientConnection_->AckAuth (entryObj, false);
	}

	void GlooxAccount::AddEntry (const QString& entryId,
			const QString& name, const QStringList& groups)
	{
		ClientConnection_->AddEntry (entryId, name, groups);
	}

	void GlooxAccount::RequestAuth (const QString& entryId,
			const QString& msg, const QString& name, const QStringList& groups)
	{
		ClientConnection_->Subscribe (entryId, msg, name, groups);
	}

	void GlooxAccount::RemoveEntry (QObject *entryObj)
	{
		auto entry = qobject_cast<GlooxCLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "is not a GlooxCLEntry";
			return;
		}

		if (entry->IsGateway ())
		{
			const auto& allEntries = ClientConnection_->GetCLEntries ();

			const auto& gwJid = entry->GetJID ();

			QList<GlooxCLEntry*> subs;
			for (auto obj : allEntries)
			{
				auto otherEntry = qobject_cast<GlooxCLEntry*> (obj);
				if (otherEntry &&
						otherEntry != entry &&
						otherEntry->GetJID ().endsWith (gwJid))
					subs << otherEntry;
			}

			if (!subs.isEmpty ())
			{
				GWItemsRemovalDialog dia (subs);
				if (dia.exec () == QDialog::Accepted)
					for (auto subEntry : subs)
						RemoveEntry (subEntry);
			}
		}

		ClientConnection_->Remove (entry);
	}

	QObject* GlooxAccount::GetTransferManager () const
	{
		return TransferManager_.get ();
	}

	QIcon GlooxAccount::GetAccountIcon () const
	{
		return AccountIcon_;
	}

	QObject* GlooxAccount::GetSelfContact () const
	{
		return ClientConnection_ ?
				ClientConnection_->GetCLEntry (SettingsHolder_->GetJID (), QString ()) :
				0;
	}

	QObject* GlooxAccount::CreateSDSession ()
	{
		return new SDSession (this);
	}

	QString GlooxAccount::GetDefaultQuery () const
	{
		return GetDefaultReqHost ();
	}

	QObject* GlooxAccount::CreateSearchSession ()
	{
		return new JabberSearchSession (this);
	}

	QString GlooxAccount::GetDefaultSearchServer () const
	{
		return GetDefaultReqHost ();
	}

	IHaveConsole::PacketFormat GlooxAccount::GetPacketFormat () const
	{
		return PacketFormat::XML;
	}

	void GlooxAccount::SetConsoleEnabled (bool enabled)
	{
		ClientConnection_->SetSignaledLog (enabled);
	}

	void GlooxAccount::SubmitPost (const Post& post)
	{
		PEPMicroblog micro (post);
		ClientConnection_->GetPubSubManager ()->PublishEvent (&micro);
	}

	void GlooxAccount::PublishTune (const QMap<QString, QVariant>& tuneInfo)
	{
		UserTune tune;
		tune.SetArtist (tuneInfo ["artist"].toString ());
		tune.SetTitle (tuneInfo ["title"].toString ());
		tune.SetSource (tuneInfo ["source"].toString ());
		tune.SetLength (tuneInfo ["length"].toInt ());

		if (tuneInfo.contains ("track"))
		{
			const int track = tuneInfo ["track"].toInt ();
			if (track > 0)
				tune.SetTrack (QString::number (track));
		}

		ClientConnection_->GetPubSubManager ()->PublishEvent (&tune);
	}

	void GlooxAccount::SetMood (const QString& moodStr, const QString& text)
	{
		UserMood mood;
		mood.SetMoodStr (moodStr);
		mood.SetText (text);

		ClientConnection_->GetPubSubManager ()->PublishEvent (&mood);
	}

	void GlooxAccount::SetActivity (const QString& general,
			const QString& specific, const QString& text)
	{
		UserActivity activity;
		activity.SetGeneralStr (general);
		activity.SetSpecificStr (specific);
		activity.SetText (text);

		ClientConnection_->GetPubSubManager ()->PublishEvent (&activity);
	}

	void GlooxAccount::SetGeolocationInfo (const GeolocationInfo_t& info)
	{
		UserLocation location;
		location.SetInfo (info);
		ClientConnection_->GetPubSubManager ()->PublishEvent (&location);
	}

	GeolocationInfo_t GlooxAccount::GetUserGeolocationInfo (QObject *obj,
			const QString& variant) const
	{
		EntryBase *entry = qobject_cast<EntryBase*> (obj);
		if (!entry)
			return GeolocationInfo_t ();

		return entry->GetGeolocationInfo (variant);
	}

#ifdef ENABLE_MEDIACALLS
	ISupportMediaCalls::MediaCallFeatures GlooxAccount::GetMediaCallFeatures () const
	{
		return MCFSupportsAudioCalls;
	}

	QObject* GlooxAccount::Call (const QString& id, const QString& variant)
	{
		if (id == qobject_cast<ICLEntry*> (GetSelfContact ())->GetEntryID ())
		{
			Core::Instance ().SendEntity (Util::MakeNotification ("LeechCraft",
						tr ("Why would you call yourself?"),
						PWarning_));

			return 0;
		}

		QString target = GlooxCLEntry::JIDFromID (this, id);

		QString var = variant;
		if (var.isEmpty ())
		{
			QObject *entryObj = GetClientConnection ()->
					GetCLEntry (target, QString ());
			GlooxCLEntry *entry = qobject_cast<GlooxCLEntry*> (entryObj);
			if (entry)
				var = entry->Variants ().value (0);
			else
				qWarning () << Q_FUNC_INFO
						<< "null entry for"
						<< target;
		}
		if (!var.isEmpty ())
			target += '/' + var;

		const auto call = new MediaCall (this, ClientConnection_->GetCallManager ()->call (target));
		emit called (call);
		return call;
	}
#endif

	void GlooxAccount::SuggestItems (QList<RIEXItem> items, QObject *to, QString message)
	{
		EntryBase *entry = qobject_cast<EntryBase*> (to);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to cast"
					<< to
					<< "to EntryBase";
			return;
		}

		QList<RIEXManager::Item> add;
		QList<RIEXManager::Item> del;
		QList<RIEXManager::Item> modify;
		Q_FOREACH (const RIEXItem& item, items)
		{
			switch (item.Action_)
			{
			case RIEXItem::AAdd:
				add << RIEXManager::Item (RIEXManager::Item::AAdd,
						item.ID_, item.Nick_, item.Groups_);
				break;
			case RIEXItem::ADelete:
				del << RIEXManager::Item (RIEXManager::Item::ADelete,
						item.ID_, item.Nick_, item.Groups_);
				break;
			case RIEXItem::AModify:
				modify << RIEXManager::Item (RIEXManager::Item::AModify,
						item.ID_, item.Nick_, item.Groups_);
				break;
			default:
				qWarning () << Q_FUNC_INFO
						<< "unknown action"
						<< item.Action_
						<< "for item"
						<< item.ID_;
				break;
			}
		}

		if (!add.isEmpty ())
			ClientConnection_->GetRIEXManager ()->SuggestItems (entry, add, message);
		if (!modify.isEmpty ())
			ClientConnection_->GetRIEXManager ()->SuggestItems (entry, modify, message);
		if (!del.isEmpty ())
			ClientConnection_->GetRIEXManager ()->SuggestItems (entry, del, message);
	}

	QWidget* GlooxAccount::GetMUCBookmarkEditorWidget ()
	{
		return new BookmarkEditWidget ();
	}

	QVariantList GlooxAccount::GetBookmarkedMUCs () const
	{
		QVariantList result;

		const QXmppBookmarkSet& set = GetBookmarks ();

		Q_FOREACH (const QXmppBookmarkConference& conf, set.conferences ())
		{
			const QStringList& split = conf.jid ().split ('@', QString::SkipEmptyParts);
			if (split.size () != 2)
			{
				qWarning () << Q_FUNC_INFO
						<< "incorrectly split jid for conf"
						<< conf.jid ()
						<< split;
				continue;
			}

			QVariantMap cm;
			cm ["HumanReadableName"] = QString ("%1 (%2)")
					.arg (conf.jid ())
					.arg (conf.nickName ());
			cm ["AccountID"] = GetAccountID ();
			cm ["Nick"] = conf.nickName ();
			cm ["Room"] = split.at (0);
			cm ["Server"] = split.at (1);
			cm ["Autojoin"] = conf.autoJoin ();
			cm ["StoredName"] = conf.name ();
			result << cm;
		}

		return result;
	}

	void GlooxAccount::SetBookmarkedMUCs (const QVariantList& datas)
	{
		QSet<QString> jids;

		QList<QXmppBookmarkConference> mucs;
		Q_FOREACH (const QVariant& var, datas)
		{
			const QVariantMap& map = var.toMap ();
			QXmppBookmarkConference conf;
			conf.setAutoJoin (map.value ("Autojoin").toBool ());

			const auto& room = map.value ("Room").toString ();
			const auto& server = map.value ("Server").toString ();
			if (room.isEmpty () || server.isEmpty ())
				continue;

			const auto& jid = room + '@' + server;
			if (jids.contains (jid))
				continue;

			jids << jid;

			conf.setJid (jid);
			conf.setNickName (map.value ("Nick").toString ());
			conf.setName (map.value ("StoredName").toString ());
			mucs << conf;
		}

		QXmppBookmarkSet set;
		set.setConferences (mucs);
		set.setUrls (GetBookmarks ().urls ());
		SetBookmarks (set);
	}

	QObject* GlooxAccount::RequestLastActivity (QObject *entry, const QString& variant)
	{
		auto jid = qobject_cast<ICLEntry*> (entry)->GetHumanReadableID ();
		if (!variant.isEmpty ())
			jid += '/' + variant;
		return RequestLastActivity (jid);
	}

	QObject* GlooxAccount::RequestLastActivity (const QString& jid)
	{
		auto pending = new PendingLastActivityRequest { jid, this };

		const auto manager = ClientConnection_->GetLastActivityManager ();
		const auto& id = manager->RequestLastActivity (jid);
		connect (manager,
				SIGNAL (gotLastActivity (QString, int)),
				pending,
				SLOT (handleGotLastActivity (QString, int)));

		ClientConnection_->GetErrorManager ()->Whitelist (id);
		ClientConnection_->AddCallback (id,
				[pending] (const QXmppIq& iq)
				{
					if (iq.type () == QXmppIq::Error)
						pending->deleteLater ();
				});

		return pending;
	}

	bool GlooxAccount::SupportsFeature (Feature f) const
	{
		switch (f)
		{
		case Feature::UpdatePass:
		case Feature::DeregisterAcc:
			return true;
		}

		return false;
	}

	void GlooxAccount::UpdateServerPassword (const QString& newPass)
	{
		if (newPass.isEmpty ())
			return;

		const auto& jid = SettingsHolder_->GetJID ();
		const auto aPos = jid.indexOf ('@');

		QXmppElement userElem;
		userElem.setTagName ("username");
		userElem.setValue (aPos > 0 ? jid.left (aPos) : jid);

		QXmppElement passElem;
		passElem.setTagName ("password");
		passElem.setValue (newPass);

		QXmppElement queryElem;
		queryElem.setTagName ("query");
		queryElem.setAttribute ("xmlns", XooxUtil::NsRegister);
		queryElem.appendChild (userElem);
		queryElem.appendChild (passElem);

		QXmppIq iq (QXmppIq::Set);
		iq.setTo (GetDefaultReqHost ());
		iq.setExtensions ({ queryElem });

		ClientConnection_->SendPacketWCallback (iq,
				[this, newPass] (const QXmppIq& reply) -> void
				{
					if (reply.type () != QXmppIq::Result)
						return;

					emit serverPasswordUpdated (newPass);
					ParentProtocol_->GetProxyObject ()->SetPassword (newPass, this);
					ClientConnection_->SetPassword (newPass);
				});
	}

	namespace
	{
		QXmppIq MakeDeregisterIq ()
		{
			QXmppElement removeElem;
			removeElem.setTagName ("remove");

			QXmppElement queryElem;
			queryElem.setTagName ("query");
			queryElem.setAttribute ("xmlns", XooxUtil::NsRegister);
			queryElem.appendChild (removeElem);

			QXmppIq iq { QXmppIq::Set };
			iq.setExtensions ({ queryElem });
			return iq;
		}
	}

	void GlooxAccount::DeregisterAccount ()
	{
		const auto worker = [this]
		{
			ClientConnection_->SendPacketWCallback (MakeDeregisterIq (),
					[this] (const QXmppIq& reply)
					{
						if (reply.type () == QXmppIq::Result)
						{
							ParentProtocol_->RemoveAccount (this);
							ChangeState ({ SOffline, {} });
						}
						else
							qWarning () << Q_FUNC_INFO
									<< "unable to cancel the registration:"
									<< reply.type ();
					});
		};

		if (GetState ().State_ != SOffline)
		{
			worker ();
			return;
		}

		ChangeState ({ SOnline, {} });
		new Util::SlotClosure<Util::ChoiceDeletePolicy>
		{
			[this, worker]
			{
				switch (GetState ().State_)
				{
				case SOffline:
				case SError:
				case SConnecting:
					return Util::ChoiceDeletePolicy::Delete::No;
				default:
					break;
				}

				worker ();

				return Util::ChoiceDeletePolicy::Delete::Yes;
			},
			this,
			SIGNAL (statusChanged (EntryStatus)),
			this
		};
	}

	bool GlooxAccount::HasFeature (ServerHistoryFeature feature) const
	{
		auto infoStorage = ClientConnection_->GetServerInfoStorage ();
		const bool supportsMam = Xep0313Manager::Supports0313 (infoStorage->GetServerFeatures ());
		switch (feature)
		{
		case ServerHistoryFeature::AccountSupportsHistory:
		case ServerHistoryFeature::Configurable:
			return supportsMam;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown feature"
				<< static_cast<int> (feature);
		return false;
	}

	void GlooxAccount::OpenServerHistoryConfiguration ()
	{
		auto dialog = new Xep0313PrefsDialog (ClientConnection_->GetXep0313Manager ());
		dialog->show ();
	}

	QAbstractItemModel* GlooxAccount::GetServerContactsModel () const
	{
		return Xep0313ModelMgr_->GetModel ();
	}

	void GlooxAccount::FetchServerHistory (const QModelIndex& index,
			const QByteArray& startId, int count)
	{
		const auto& jid = Xep0313ModelMgr_->Index2Jid (index);
		ClientConnection_->GetXep0313Manager ()->RequestHistory (jid, startId, count);
	}

	DefaultSortParams GlooxAccount::GetSortParams () const
	{
		return { 0, Qt::DisplayRole, Qt::AscendingOrder };
	}

	QFuture<IHaveServerHistory::DatedFetchResult_t> GlooxAccount::FetchServerHistory (const QDateTime&)
	{
		return {};
	}

	bool GlooxAccount::SupportsBlacklists () const
	{
		if (!ClientConnection_)
			return false;

		return ClientConnection_->GetPrivacyListsManager ()->IsSupported ();
	}

	void GlooxAccount::SuggestToBlacklist (const QList<ICLEntry*>& entries)
	{
		if (!ClientConnection_)
		{
			qWarning () << Q_FUNC_INFO
					<< "no client connection is instantiated";
			return;
		}

		bool ok = false;
		const QStringList variants { tr ("By full JID"), tr ("By domain") };
		const auto& selected = QInputDialog::getItem (nullptr,
				"LeechCraft",
				tr ("Select block type:"),
				variants,
				0,
				false,
				&ok);
		if (!ok)
			return;

		QStringList allJids { Util::Map (entries, [] (ICLEntry *entry) { return entry->GetHumanReadableID (); }) };
		if (variants.indexOf (selected) == 1)
			allJids = Util::Map (allJids,
					[] (const QString& jid)
					{
						QString bare;
						ClientConnection::Split (jid, &bare, nullptr);
						return bare.section ('@', 1);
					});

		allJids.removeDuplicates ();

		new AddToBlockedRunner { allJids, ClientConnection_, this };
	}

#ifdef ENABLE_CRYPT
	void GlooxAccount::SetPrivateKey (const QCA::PGPKey& key)
	{
		ClientConnection_->GetCryptHandler ()->GetPGPManager ()->SetPrivateKey (key);
	}

	QCA::PGPKey GlooxAccount::GetPrivateKey () const
	{
		return ClientConnection_->GetCryptHandler ()->GetPGPManager ()->PrivateKey ();
	}

	void GlooxAccount::SetEntryKey (QObject *entryObj, const QCA::PGPKey& pubKey)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "doesn't implement ICLEntry";
			return;
		}

		auto mgr = ClientConnection_->GetCryptHandler ()->GetPGPManager ();
		mgr->SetPublicKey (entry->GetHumanReadableID (), pubKey);
	}

	QCA::PGPKey GlooxAccount::GetEntryKey (QObject *entryObj) const
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "doesn't implement ICLEntry";
			return QCA::PGPKey ();
		}

		auto mgr = ClientConnection_->GetCryptHandler ()->GetPGPManager ();
		return mgr->PublicKey (entry->GetHumanReadableID ());
	}

	GPGExceptions::MaybeException_t GlooxAccount::SetEncryptionEnabled (QObject *entry, bool enabled)
	{
		using EitherException_t = Util::Either<GPGExceptions::AnyException_t, Util::Void>;

		const auto glEntry = qobject_cast<GlooxCLEntry*> (entry);

		const auto cryptHandler = ClientConnection_->GetCryptHandler ();
		const auto pgpManager = cryptHandler->GetPGPManager ();

		bool beenChanged = false;
		const auto emitGuard = Util::MakeScopeGuard ([&]
					{ emit encryptionStateChanged (entry, beenChanged ? enabled : !enabled); });

		const auto& result = EitherException_t::Right ({}) >>
				[=] (const Util::Void&)
				{
					return glEntry ?
							EitherException_t::Right ({}) :
							EitherException_t::Left (GPGExceptions::General { "Null entry" });
				} >>
				[=] (const Util::Void&)
				{
					return enabled && pgpManager->PublicKey (glEntry->GetJID ()).isNull () ?
							EitherException_t::Left (GPGExceptions::NullPubkey {}) :
							EitherException_t::Right ({});
				} >>
				[=, &beenChanged] (const Util::Void&)
				{
					if (!cryptHandler->SetEncryptionEnabled (glEntry->GetJID (), enabled))
						return EitherException_t::Left (GPGExceptions::General { "Cannot change encryption state. "});

					beenChanged = true;
					return EitherException_t::Right ({});
				};
		return result.MaybeLeft ();
	}

	bool GlooxAccount::IsEncryptionEnabled (QObject *entry) const
	{
		const auto glEntry = qobject_cast<GlooxCLEntry*> (entry);
		if (!glEntry)
			return false;

		return ClientConnection_->GetCryptHandler ()->IsEncryptionEnabled (glEntry->GetJID ());
	}
#endif

	QString GlooxAccount::GetNick () const
	{
		return SettingsHolder_->GetNick ();
	}

	namespace
	{
		QString NormalizeRoomJid (const QString& jid)
		{
			return jid.toLower ();
		}
	}

	void GlooxAccount::JoinRoom (const QString& origJid, const QString& nick, const QString& password)
	{
		if (!ClientConnection_)
		{
			qWarning () << Q_FUNC_INFO
					<< "null ClientConnection";
			return;
		}

		auto jid = NormalizeRoomJid (origJid);
		if (jid != origJid)
			qWarning () << Q_FUNC_INFO
					<< "room jid normalization happened from"
					<< origJid
					<< "to"
					<< jid;

		const auto existingObj = ClientConnection_->GetCLEntry (jid, {});
		const auto existing = qobject_cast<ICLEntry*> (existingObj);
		if (existing && existing->GetEntryType () != ICLEntry::EntryType::MUC)
		{
			const auto res = QMessageBox::question (nullptr,
					"LeechCraft",
					tr ("Cannot join something that's already added to the roster. "
						"Do you want to remove %1 from roster and retry?")
						.arg ("<em>" + jid + "</em>"),
					QMessageBox::Yes | QMessageBox::No);
			if (res != QMessageBox::Yes)
				return;

			RemoveEntry (existingObj);
			ExistingEntry2JoinConflict_ [existingObj] = qMakePair (jid, nick);
			return;
		}

		const auto entry = ClientConnection_->JoinRoom (jid, nick);
		if (!entry)
			return;

		if (!password.isEmpty ())
			entry->GetRoomHandler ()->GetRoom ()->setPassword (password);

		emit gotCLItems ({ entry });
	}

	void GlooxAccount::JoinRoom (const QString& server,
			const QString& room, const QString& nick, const QString& password)
	{
		const auto& jidStr = room + '@' + server;
		JoinRoom (jidStr, nick, password);
	}

	std::shared_ptr<ClientConnection> GlooxAccount::GetClientConnection () const
	{
		return ClientConnection_;
	}

	GlooxCLEntry* GlooxAccount::CreateFromODS (OfflineDataSource_ptr ods)
	{
		return ClientConnection_->AddODSCLEntry (ods);
	}

	QXmppBookmarkSet GlooxAccount::GetBookmarks () const
	{
		if (!ClientConnection_)
			return QXmppBookmarkSet ();

		return ClientConnection_->GetBookmarks ();
	}

	void GlooxAccount::SetBookmarks (const QXmppBookmarkSet& set)
	{
		if (!ClientConnection_)
			return;

		ClientConnection_->SetBookmarks (set);
	}

	void GlooxAccount::UpdateOurPhotoHash (const QByteArray& hash)
	{
		SettingsHolder_->SetPhotoHash (hash);
	}

	void GlooxAccount::CreateSDForResource (const QString& resource)
	{
		auto sd = new SDSession (this);
		sd->SetQuery (resource);
		emit gotSDSession (sd);
	}

	void GlooxAccount::RequestRosterSave ()
	{
		emit rosterSaveRequested ();
	}

	QByteArray GlooxAccount::Serialize () const
	{
		quint16 version = 9;

		QByteArray result;
		{
			QDataStream ostr (&result, QIODevice::WriteOnly);
			ostr << version
				<< Name_;
			SettingsHolder_->Serialize (ostr);
		}

		return result;
	}

	GlooxAccount* GlooxAccount::Deserialize (const QByteArray& data, GlooxProtocol *proto)
	{
		quint16 version = 0;

		QDataStream in (data);
		in >> version;

		if (version < 1 || version > 9)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return 0;
		}

		QString name;
		in >> name;
		GlooxAccount *result = new GlooxAccount (name, proto, proto);
		result->GetSettings ()->Deserialize (in, version);
		result->Init ();

		return result;
	}

	GlooxMessage* GlooxAccount::CreateMessage (IMessage::Type type,
			const QString& variant,
			const QString& body,
			const QString& jid)
	{
		return ClientConnection_->CreateMessage (type, variant, body, jid);
	}

	QString GlooxAccount::GetPassword (bool authfailure)
	{
		return ParentProtocol_->GetProxyObject ()->GetAccountPassword (this, !authfailure);
	}

	void GlooxAccount::regenAccountIcon (const QString& jid)
	{
		AccountIcon_ = QIcon ();

		if (jid.contains ("google") ||
				jid.contains ("gmail"))
			AccountIcon_ = QIcon (":/plugins/azoth/plugins/xoox/resources/images/special/gtalk.svg");
		else if (jid.contains ("facebook") ||
				jid.contains ("fb.com"))
			AccountIcon_ = QIcon (":/plugins/azoth/plugins/xoox/resources/images/special/facebook.svg");
		else if (jid.contains ("odnoklassniki"))
			AccountIcon_ = QIcon (":/plugins/azoth/plugins/xoox/resources/images/special/odnoklassniki.svg");
	}

	QString GlooxAccount::GetDefaultReqHost () const
	{
		const auto& second = SettingsHolder_->GetJID ().split ('@', QString::SkipEmptyParts).value (1);
		const int slIdx = second.indexOf ('/');
		return slIdx >= 0 ? second.left (slIdx) : second;
	}

	void GlooxAccount::handleEntryRemoved (QObject *entry)
	{
		emit removedCLItems ({ entry });

		if (ExistingEntry2JoinConflict_.contains (entry))
		{
			const auto& pair = ExistingEntry2JoinConflict_.take (entry);
			JoinRoom (pair.first, pair.second, {});
		}
	}

	void GlooxAccount::handleGotRosterItems (const QList<QObject*>& items)
	{
		emit gotCLItems (items);
	}

	void GlooxAccount::handleServerAuthFailed ()
	{
		const QString& pwd = GetPassword (true);
		if (!pwd.isNull ())
		{
			ClientConnection_->SetPassword (pwd);
			ClientConnection_->SetState (ClientConnection_->GetLastState ());
		}
	}

	void GlooxAccount::feedClientPassword ()
	{
		ClientConnection_->SetPassword (GetPassword ());
	}

	void GlooxAccount::showSelfVCard ()
	{
		if (!ClientConnection_)
			return;

		const auto& jid = SettingsHolder_->GetJID ();
		auto entry = qobject_cast<EntryBase*> (ClientConnection_->GetCLEntry (jid));
		if (entry)
			entry->ShowInfo ();
	}

	void GlooxAccount::showPrivacyDialog ()
	{
		auto mgr = ClientConnection_->GetPrivacyListsManager ();
		auto plcd = new PrivacyListsConfigDialog (mgr);
		plcd->show ();
	}

	void GlooxAccount::handleCarbonsToggled (bool toggled)
	{
		SettingsHolder_->SetMessageCarbonsEnabled (toggled);
	}

	void GlooxAccount::handleServerHistoryFetched (const QString& jid,
			const QString& id, SrvHistMessages_t messages)
	{
		const auto& index = Xep0313ModelMgr_->Jid2Index (jid);

		const auto& ourNick = GetOurNick ();

		const auto jidEntry = ClientConnection_->GetCLEntry (jid);
		const auto& otherNick = jidEntry ?
				qobject_cast<ICLEntry*> (jidEntry)->GetHumanReadableID () :
				jid;
		for (auto& message : messages)
			message.Nick_ = message.Dir_ == IMessage::Direction::In ?
					otherNick :
					ourNick;

		emit serverHistoryFetched (index, id.toUtf8 (), messages);
	}

#ifdef ENABLE_MEDIACALLS
	void GlooxAccount::handleIncomingCall (QXmppCall *call)
	{
		emit called (new MediaCall (this, call));
	}
#endif
}
}
}
