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

#include "msgwrapper.h"
#include <PendingSendMessage>
#include <util/xpc/util.h>
#include "entrywrapper.h"

namespace LC
{
namespace Azoth
{
namespace Astrality
{
	MsgWrapper::MsgWrapper (const Tp::ReceivedMessage& msg,
			Tp::ContactMessengerPtr messenger, EntryWrapper *entry)
	: QObject (entry)
	, Messenger_ (messenger)
	, Entry_ (entry)
	, Body_ (msg.text ())
	, DT_ (msg.received ())
	, Dir_ (Direction::In)
	, MT_ (Type::ChatMessage)
	, MST_ (SubType::Other)
	{
	}

	MsgWrapper::MsgWrapper (const QString& body, Direction dir,
			Tp::ContactMessengerPtr messenger, EntryWrapper *entry,
			Type mt, SubType mst)
	: QObject (entry)
	, Messenger_ (messenger)
	, Entry_ (entry)
	, Body_ (body)
	, DT_ (QDateTime::currentDateTime ())
	, Dir_ (dir)
	, MT_ (mt)
	, MST_ (mst)
	{
		connect (this,
				SIGNAL (gotEntity (LC::Entity)),
				Entry_,
				SIGNAL (gotEntity (LC::Entity)));
	}

	QObject* MsgWrapper::GetQObject ()
	{
		return this;
	}

	void MsgWrapper::Send ()
	{
		Store ();
		connect (Messenger_->sendMessage (Body_),
				SIGNAL (finished (Tp::PendingOperation*)),
				this,
				SLOT (handleMessageSent (Tp::PendingOperation*)));
	}

	void MsgWrapper::Store ()
	{
		Entry_->HandleMessage (this);
	}

	IMessage::Direction MsgWrapper::GetDirection () const
	{
		return Dir_;
	}

	IMessage::Type MsgWrapper::GetMessageType () const
	{
		return MT_;
	}

	IMessage::SubType MsgWrapper::GetMessageSubType () const
	{
		return MST_;
	}

	QObject* MsgWrapper::OtherPart () const
	{
		return Entry_;
	}

	QString MsgWrapper::GetOtherVariant () const
	{
		return QString ();
	}

	QString MsgWrapper::GetBody () const
	{
		return Body_;
	}

	void MsgWrapper::SetBody (const QString& body)
	{
		Body_ = body;
	}

	QDateTime MsgWrapper::GetDateTime () const
	{
		return DT_;
	}

	void MsgWrapper::SetDateTime (const QDateTime& dt)
	{
		DT_ = dt;
	}

	void MsgWrapper::handleMessageSent (Tp::PendingOperation *po)
	{
		auto psm = qobject_cast<Tp::PendingSendMessage*> (po);
		if (psm->isError ())
		{
			qWarning () << Q_FUNC_INFO
					<< psm->errorName ()
					<< psm->errorMessage ();
			emit gotEntity (Util::MakeNotification ("Azoth",
					tr ("Unable to send message to %1: %2 (%3).")
						.arg (Entry_->GetEntryName ())
						.arg (po->errorName ())
						.arg (po->errorMessage ()),
					Priority::Critical));
			return;
		}
	}
}
}
}
