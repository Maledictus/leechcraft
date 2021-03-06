/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include "ircserverclentry.h"
#include <QAction>
#include "ircaccount.h"
#include "ircmessage.h"
#include "ircserverhandler.h"
#include "clientconnection.h"
#include "ircparser.h"
#include "servercommandmessage.h"
#include "serverinfowidget.h"

namespace LC
{
namespace Azoth
{
namespace Acetamide
{
	IrcServerCLEntry::IrcServerCLEntry (IrcServerHandler *handler,
			IrcAccount *account)
	: EntryBase (account)
	, ISH_ (handler)
	{
		QAction *showChannels = new QAction (tr ("Channels list"), this);
		connect (showChannels,
				SIGNAL (triggered ()),
				ISH_,
				SLOT (showChannels ()));
		Actions_ << showChannels;
	}

	IrcServerHandler* IrcServerCLEntry::GetIrcServerHandler () const
	{
		return ISH_;
	}

	IrcAccount* IrcServerCLEntry::GetIrcAccount () const
	{
		return Account_;
	}

	IAccount* IrcServerCLEntry::GetParentAccount () const
	{
		return Account_;
	}

	ICLEntry::Features IrcServerCLEntry::GetEntryFeatures () const
	{
		return FSessionEntry;
	}

	ICLEntry::EntryType IrcServerCLEntry::GetEntryType () const
	{
		return EntryType::MUC;
	}

	QString IrcServerCLEntry::GetEntryID () const
	{
		return Account_->GetAccountID () + "_" + ISH_->GetServerID ();
	}

	QString IrcServerCLEntry::GetEntryName () const
	{
		return ISH_->GetServerID ();
	}

	void IrcServerCLEntry::SetEntryName (const QString&)
	{
	}

	QStringList IrcServerCLEntry::Groups () const
	{
		return QStringList () << tr ("Servers");
	}

	void IrcServerCLEntry::SetGroups (const QStringList&)
	{
	}

	QStringList IrcServerCLEntry::Variants () const
	{
		QStringList result;
		result << "";
		return result;
	}

	IMessage* IrcServerCLEntry::CreateMessage (IMessage::Type,
			const QString& variant, const QString& body)
	{
		if (variant.isEmpty ())
			return new ServerCommandMessage (body, this);
		else
			return nullptr;
	}

	IMUCEntry::MUCFeatures IrcServerCLEntry::GetMUCFeatures () const
	{
		return 0;
	}

	QString IrcServerCLEntry::GetMUCSubject () const
	{
		return {};
	}

	bool IrcServerCLEntry::CanChangeSubject () const
	{
		return false;
	}

	void IrcServerCLEntry::SetMUCSubject (const QString&)
	{
	}

	QList<QObject*> IrcServerCLEntry::GetParticipants ()
	{
		return QList<QObject*> ();
	}

	bool IrcServerCLEntry::IsAutojoined () const
	{
		return false;
	}

	void IrcServerCLEntry::Join ()
	{
		ISH_->GetParser ()->NickCommand (QStringList () << ISH_->GetNickName ());
	}

	void IrcServerCLEntry::Leave (const QString&)
	{
		ISH_->SendQuit ();
	}

	QString IrcServerCLEntry::GetNick () const
	{
		return ISH_->GetNickName ();
	}

	void IrcServerCLEntry::SetNick (const QString& nick)
	{
		ISH_->SetNickName (nick);
	}

	QString IrcServerCLEntry::GetGroupName () const
	{
		return QString ();
	}

	QString IrcServerCLEntry::GetRealID (QObject*) const
	{
		return QString ();
	}

	QVariantMap IrcServerCLEntry::GetIdentifyingData () const
	{
		QVariantMap result;
		result ["HumanReadableName"] = QString ("%1 on %2:%3")
				.arg (ISH_->GetNickName ())
				.arg (ISH_->GetServerOptions ().ServerName_)
				.arg (ISH_->GetServerOptions ().ServerPort_);
		result ["AccountID"] = ISH_->
				GetAccount ()->GetAccountID ();
		result ["Nickname"] = ISH_->
				GetNickName ();
		result ["Server"] = ISH_->GetServerOptions ().ServerName_;
		result ["Port"] = ISH_->GetServerOptions ().ServerPort_;
		result ["Encoding"] = ISH_->GetServerOptions ().ServerEncoding_;
		result ["SSL"] = ISH_->GetServerOptions ().SSL_;

		return result;
	}

	void IrcServerCLEntry::InviteToMUC (const QString&, const QString&)
	{
	}

	QWidget* IrcServerCLEntry::GetConfigurationWidget ()
	{
		return new ServerInfoWidget (this);
	}

	void IrcServerCLEntry::AcceptConfiguration (QWidget*)
	{
		// there is nothing to implement
	}

	QMap<QString, QString> IrcServerCLEntry::GetISupport () const
	{
		return ISH_->GetISupport ();
	}

}
}
}
