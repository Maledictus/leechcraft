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

#pragma once

#include <memory>
#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <QMetaType>
#include <QPair>
#include <QHash>
#include <QSet>
#include <vmime/header.hpp>
#include "attdescr.h"
#include "address.h"

namespace LeechCraft
{
namespace Snails
{
	class Message
	{
		QByteArray FolderID_;
		QByteArray MessageID_;
		QList<QStringList> Folders_;
		quint64 Size_ = 0;
		QDateTime Date_;
		QString Subject_;

		QString Body_;
		QString HTMLBody_;

		QList<QByteArray> InReplyTo_;
		QList<QByteArray> References_;

		bool IsRead_ = false;

		QList<AttDescr> Attachments_;
	private:
		QHash<AddressType, Addresses_t> Addresses_;
	public:
		bool IsFullyFetched () const;

		/** @brief Returns folder-specific message ID.
		 */
		QByteArray GetFolderID () const;

		/** @brief Sets folder-specific message ID.
		 */
		void SetFolderID (const QByteArray& id);

		QByteArray GetMessageID () const;
		void SetMessageID (const QByteArray&);

		QList<QStringList> GetFolders () const;
		void AddFolder (const QStringList&);
		void SetFolders (const QList<QStringList>&);

		quint64 GetSize () const;
		void SetSize (quint64);

		Address GetAddress (AddressType) const;
		Addresses_t GetAddresses (AddressType) const;
		QHash<AddressType, Addresses_t> GetAddresses () const;
		bool HasAddress (AddressType) const;
		void AddAddress (AddressType, const Address&);
		void SetAddress (AddressType, const Address&);
		void SetAddresses (AddressType, const Addresses_t&);

		QString GetAddressString (AddressType) const;

		QDateTime GetDate () const;
		void SetDate (const QDateTime&);

		QString GetSubject () const;
		void SetSubject (const QString&);

		QString GetBody () const;
		void SetBody (const QString&);

		QString GetHTMLBody () const;
		void SetHTMLBody (const QString&);

		QList<QByteArray> GetInReplyTo () const;
		void SetInReplyTo (const QList<QByteArray>&);
		void AddInReplyTo (const QByteArray&);

		QList<QByteArray> GetReferences () const;
		void SetReferences (const QList<QByteArray>&);
		void AddReferences (const QByteArray&);

		bool IsRead () const;
		void SetRead (bool);

		QList<AttDescr> GetAttachments () const;
		void AddAttachment (const AttDescr&);
		void SetAttachmentList (const QList<AttDescr>&);

		void Dump () const;

		QByteArray Serialize () const;
		void Deserialize (const QByteArray&);
	};

	typedef std::shared_ptr<Message> Message_ptr;
	typedef QSet<Message_ptr> MessageSet;

	uint qHash (const Message_ptr);

	using MessageWHeaders_t = std::pair<Message_ptr, vmime::shared_ptr<const vmime::header>>;
}
}

Q_DECLARE_METATYPE (LeechCraft::Snails::Message_ptr)
Q_DECLARE_METATYPE (QList<LeechCraft::Snails::Message_ptr>)
