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

#include "packetfactory.h"
#include <QCryptographicHash>
#include <QtDebug>
#include <QStringList>
#include "headers.h"
#include "conversions.h"
#include "halfpacket.h"

namespace LC
{
namespace Azoth
{
namespace Vader
{
namespace Proto
{
	PacketFactory::PacketFactory ()
	: Seq_ (0)
	{
	}

	Packet PacketFactory::Hello ()
	{
		return HalfPacket { Header (Packets::Hello, Seq_++), QByteArray () };
	}

	Packet PacketFactory::Ping ()
	{
		return HalfPacket { Header (Packets::Ping, Seq_++), QByteArray () };
	}

	Packet PacketFactory::Login (const QString& login,
			const QString& pass,
			quint32 state,
			const QString& status,
			const QString& ua)
	{
		const QByteArray& data = ToMRIM (ToMRIM1251 (login),
				QCryptographicHash::hash (ToMRIM1251 (pass), QCryptographicHash::Md5),
				state,
				QByteArray (),
				QByteArray (),
				ToMRIM16 (status),
				static_cast<quint32> (FeatureFlag::BaseSmiles | FeatureFlag::Wakeup),
				ToMRIM1251 (ua),
				QByteArray ("ru"),
				0,
				0,
				QByteArray ("vader"));
		return HalfPacket { Header (Packets::Login2, Seq_++), data };
	}

	Packet PacketFactory::SetStatus (quint32 state, const QString& status)
	{
		const QByteArray& data = ToMRIM (state,
				QByteArray (),
				QByteArray (),
				ToMRIM16 (status));
		return HalfPacket { Header (Packets::ChangeStatus, Seq_++), data };
	}

	Packet PacketFactory::RequestInfo (const QString& id)
	{
		const QStringList& split = id.split ("@", QString::SkipEmptyParts);
		const QByteArray& data = ToMRIM (static_cast<quint32> (WPParams::User),
				split.value (0),
				static_cast<quint32> (WPParams::Domain),
				split.value (1));
		return HalfPacket { Header (Packets::WPRequest, Seq_++), data };
	}

	Packet PacketFactory::Message (MsgFlags flags,
			const QString& to, const QString& msg)
	{
		const QByteArray& data = ToMRIM (static_cast<quint32> (flags), to, ToMRIM16 (msg), ToMRIM1251 (" "));
		return HalfPacket { Header (Packets::Msg, Seq_++), data };
	}

	Packet PacketFactory::MessageAck (const QString& from, quint32 msgId)
	{
		const QByteArray& data = ToMRIM (from, msgId);
		return HalfPacket { Header (Packets::MsgRecv, Seq_++), data };
	}

	Packet PacketFactory::OfflineMessageAck (const UIDL& id)
	{
		const QByteArray& data = ToMRIM (id);
		return HalfPacket { Header (Packets::DeleteOfflineMsg, Seq_++), data };
	}

	Packet PacketFactory::Microblog (BlogStatus st, const QString& text)
	{
		const QByteArray& data = ToMRIM (static_cast<quint32> (st), ToMRIM16 (text));
		return HalfPacket { Header (Packets::MicroblogPost, Seq_++), data };
	}

	Packet PacketFactory::SMS (const QString& to, const QString& text)
	{
		const QByteArray& data = ToMRIM (0, to, ToMRIM16 (text));
		return HalfPacket { Header (Packets::SMS, Seq_++), data };
	}

	Packet PacketFactory::AddGroup (const QString& name, int numGroups)
	{
		const QByteArray& data = ToMRIM (static_cast<quint32> (ContactOpFlag::Group | (numGroups << 24)),
				0, QString (), ToMRIM16 (name), QString (), 0, 0);
		return HalfPacket { Header (Packets::Contact, Seq_++), data };
	}

	Packet PacketFactory::AddContact (ContactOpFlags flags,
			quint32 group, const QString& email, const QString& name)
	{
		const QByteArray& data = ToMRIM (static_cast<quint32> (flags), group,
				email, ToMRIM16 (name),
				QString (""), QString (" "), 0);
		return HalfPacket { Header (Packets::Contact, Seq_++), data };
	}

	Packet PacketFactory::ModifyContact (quint32 cid, ContactOpFlags flags,
			quint32 group, const QString& email, const QString& name, const QString& phone)
	{
		const QByteArray& data = ToMRIM (cid, static_cast<quint32> (flags),
				group, email, ToMRIM16 (name), phone);
		return HalfPacket { Header (Packets::ModifyContact, Seq_++), data };
	}

	Packet PacketFactory::RemoveContact (quint32 id, const QString& email, const QString& name)
	{
		const QByteArray& data = ToMRIM (id,
				static_cast<quint32> (ContactOpFlag::Removed),
				0,
				email,
				name,
				QString (" "));
		return HalfPacket { Header (Packets::ModifyContact, Seq_++), data };
	}

	Packet PacketFactory::Authorize (const QString& email)
	{
		const QByteArray& data = ToMRIM (email);
		return HalfPacket { Header (Packets::Authorize), data };
	}

	Packet PacketFactory::RequestKey ()
	{
		return HalfPacket { Header (Packets::GetMPOPSession), QByteArray () };
	}
}
}
}
}
