/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "capsmanager.h"
#include <QXmppDiscoveryManager.h>
#include "clientconnection.h"
#include "capsdatabase.h"
#include "core.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	CapsManager::CapsManager (QXmppDiscoveryManager *mgr,
			ClientConnection *connection, CapsDatabase *db)
	: QObject (connection)
	, Connection_ (connection)
	, DiscoMgr_ (mgr)
	, DB_ (db)
	{
		Caps2String_ ["http://etherx.jabber.org/streams"] = "stream";
		Caps2String_ ["urn:ietf:params:xml:ns:xmpp-tls"] = "TLS";
		Caps2String_ ["urn:ietf:params:xml:ns:xmpp-sasl"] = "SASL";
		Caps2String_ ["urn:ietf:params:xml:ns:xmpp-bind"] = "Bind";
		Caps2String_ ["urn:ietf:params:xml:ns:xmpp-session"] = "session";
		Caps2String_ ["urn:ietf:params:xml:ns:xmpp-stanzas"] = "stanzas";
		Caps2String_ ["vcard-temp"] = "XEP-0054: vCards";
		Caps2String_ ["vcard-temp:x:update"] = "XEP-0054: vCard update";
		Caps2String_ ["http://jabber.org/protocol/address"] = "XEP-0033: Extended Stanza Addressing";
		Caps2String_ ["http://jabber.org/features/iq-auth"] = "XEP-0078: non-SASL authentication (features)";
		Caps2String_ ["http://jabber.org/protocol/caps"] = "XEP-0115: Entity Capabilities";
		Caps2String_ ["http://jabber.org/protocol/compress"] = "XEP-0138: Stream Compression";
		Caps2String_ ["http://jabber.org/features/compress"] = "XEP-0138: Stream Compression (features)";
		Caps2String_ ["http://jabber.org/protocol/disco#info"] = "XEP-0030: Service Discovery (info)";
		Caps2String_ ["http://jabber.org/protocol/disco#items"] = "XEP-0030: Service Discovery (items)";
		Caps2String_ ["http://jabber.org/protocol/ibb"] = "XEP-0047: In-Band Bytestreams";
		Caps2String_ ["http://jabber.org/protocol/muc"] = "XEP-0045: Multi-User Chat";
		Caps2String_ ["http://jabber.org/protocol/muc#admin"] = "XEP-0045: Multi-User Chat (Admin)";
		Caps2String_ ["http://jabber.org/protocol/muc#owner"] = "XEP-0045: Multi-User Chat (Owner)";
		Caps2String_ ["http://jabber.org/protocol/muc#user"] = "XEP-0045: Multi-User Chat (User)";
		Caps2String_ ["http://jabber.org/protocol/chatstates"] = "XEP-0085: Chat State Notifications";
		Caps2String_ ["http://jabber.org/protocol/si"] = "XEP-0096: SI File Transfer";
		Caps2String_ ["http://jabber.org/protocol/si/profile/file-transfer"] = "XEP-0096: SI File Transfer (File Transfer profile)";
		Caps2String_ ["http://jabber.org/protocol/feature-neg"] = "XEP-0020: Feature Negotiation";
		Caps2String_ ["http://jabber.org/protocol/bytestreams"] = "XEP-0065: SOCKS5 Bytestreams";
		Caps2String_ ["http://jabber.org/protocol/xhtml-im"] = "XEP-0071: XHTML-IM";
		Caps2String_ ["http://jabber.org/protocol/rosterx"] = "XEP-0144: Roster Item Exchange";
		Caps2String_ ["http://jabber.org/protocol/commands"] = "XEP-0050: Ad-Hoc Commands";
		Caps2String_ ["http://jabber.org/protocol/pubsub"] = "XEP-0060: Publish-Subscribe";
		Caps2String_ ["http://jabber.org/protocol/activity"] = "XEP-0108: User Activity";
		Caps2String_ ["http://jabber.org/protocol/activity+notify"] = "Notifications about User Activity";
		Caps2String_ ["http://jabber.org/protocol/geoloc"] = "XEP-0080: User Location";
		Caps2String_ ["http://jabber.org/protocol/geoloc+notify"] = "Notifications about User Location";
		Caps2String_ ["http://jabber.org/protocol/mood"] = "XEP-0107: User Mood";
		Caps2String_ ["http://jabber.org/protocol/mood+notify"] = "Notifications about User Mood";
		Caps2String_ ["http://jabber.org/protocol/tune"] = "XEP-0118: User Tune";
		Caps2String_ ["http://jabber.org/protocol/tune+notify"] = "Notifications about User Tune";
		Caps2String_ ["urn:xmpp:receipts"] = "XEP-0184: Message Delivery Receipts";
		Caps2String_ ["urn:xmpp:ping"] = "XEP-0199: XMPP Ping";
		Caps2String_ ["urn:xmpp:delay"] = "XEP-0203: Delayed Delivery";
		Caps2String_ ["urn:xmpp:attention:0"] = "XEP-0224: Attention";
		Caps2String_ ["urn:xmpp:bob"] = "XEP-0231: Bits of Binary";
		Caps2String_ ["urn:xmpp:time"] = "XEP-0202: Entity Time";
		Caps2String_ ["urn:xmpp:avatar:data"] = "XEP-0084: User Avatar";
		Caps2String_ ["urn:xmpp:avatar:metadata"] = "XEP-0084: User Avatar (metadata)";
		Caps2String_ ["urn:xmpp:avatar:metadata+notify"] = "Notifications about User Avatar";
		Caps2String_ ["jabber:iq:rpc"] = "XEP-0009: Jabber-RPC";
		Caps2String_ ["jabber:iq:auth"] = "XEP-0078: non-SASL authentication";
		Caps2String_ ["jabber:iq:roster"] = "Jabber Roster";
		Caps2String_ ["jabber:iq:last"] = "XEP-0012: Last Activity";
		Caps2String_ ["jabber:iq:privacy"] = "XEP-0016: Privacy Lists";
		Caps2String_ ["jabber:iq:version"] = "XEP-0092: Software Version";
		Caps2String_ ["jabber:iq:time"] = "XEP-0090: Legacy Entity Time";
		Caps2String_ ["jabber:x:conference"] = "XEP-0249: Direct MUC Invitations";
		Caps2String_ ["jabber:x:delay"] = "XEP-0091: Legacy Delayed Delivery";
		Caps2String_ ["jabber:client"] = "client";
		Caps2String_ ["jabber:server"] = "server";
		Caps2String_ ["jabber:server:dialback"] = "XEP-0220: Server Dialback";

		connect (DiscoMgr_,
				SIGNAL (infoReceived (const QXmppDiscoveryIq&)),
				this,
				SLOT (handleInfoReceived (const QXmppDiscoveryIq&)));
		connect (DiscoMgr_,
				SIGNAL (itemsReceived (const QXmppDiscoveryIq&)),
				this,
				SLOT (handleItemsReceived (const QXmppDiscoveryIq&)));
	}

	void CapsManager::FetchCaps (const QString& jid, const QByteArray& verNode)
	{
		if (!DB_->Contains (verNode) &&
				verNode.size () > 17)	// 17 is some random number a bit less than 15
			Connection_->RequestInfo (jid);
	}

	QStringList CapsManager::GetRawCaps (const QByteArray& verNode) const
	{
		return DB_->Get (verNode);
	}

	QStringList CapsManager::GetCaps (const QByteArray& verNode) const
	{
		return GetCaps (GetRawCaps (verNode));
	}

	QStringList CapsManager::GetCaps (const QStringList& features) const
	{
		QStringList result;
		for (const auto& raw : features)
			result << Caps2String_.value (raw, raw);
		result.removeAll (QString ());
		return result;
	}

	void CapsManager::SetIdentities (const QByteArray& verNode, const QList<QXmppDiscoveryIq::Identity>& ids)
	{
		DB_->SetIdentities (verNode, ids);
	}

	QList<QXmppDiscoveryIq::Identity> CapsManager::GetIdentities (const QByteArray& verNode) const
	{
		return DB_->GetIdentities (verNode);
	}

	void CapsManager::handleInfoReceived (const QXmppDiscoveryIq& iq)
	{
		if (iq.type () != QXmppIq::Result)
			return;

		if (!iq.features ().isEmpty ())
			DB_->Set (iq.verificationString (), iq.features ());
		if (!iq.identities ().isEmpty ())
			DB_->SetIdentities (iq.verificationString (), iq.identities ());
	}

	void CapsManager::handleItemsReceived (const QXmppDiscoveryIq& iq)
	{
		if (iq.type () != QXmppIq::Result)
			return;

		if (!iq.features ().isEmpty ())
			DB_->Set (iq.verificationString (), iq.features ());
	}
}
}
}
