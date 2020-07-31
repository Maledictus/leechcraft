/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXACCOUNTCONFIGURATIONWIDGET_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXACCOUNTCONFIGURATIONWIDGET_H
#include <QWidget>
#include <QXmppTransferManager.h>
#include <QXmppConfiguration.h>
#include "ui_glooxaccountconfigurationwidget.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	class GlooxAccountConfigurationWidget : public QWidget
	{
		Q_OBJECT

		Ui::GlooxAccountConfigurationWidget Ui_;

		QString Password_;
	public:
		GlooxAccountConfigurationWidget (QWidget* = 0);

		QString GetJID () const;
		void SetJID (const QString&);
		QString GetNick () const;
		void SetNick (const QString&);
		QString GetResource () const;
		void SetResource (const QString&);
		short GetPriority () const;
		void SetPriority (short);

		QString GetHost () const;
		void SetHost (const QString&);
		int GetPort () const;
		void SetPort (int);

		int GetKAInterval () const;
		void SetKAInterval (int);
		int GetKATimeout () const;
		void SetKATimeout (int);

		bool GetFileLogEnabled () const;
		void SetFileLogEnabled (bool);

		QXmppConfiguration::StreamSecurityMode GetTLSMode () const;
		void SetTLSMode (QXmppConfiguration::StreamSecurityMode);

		QXmppTransferJob::Methods GetFTMethods () const;
		void SetFTMethods (QXmppTransferJob::Methods);

		bool GetUseSOCKS5Proxy () const;
		void SetUseSOCKS5Proxy (bool);

		QString GetSOCKS5Proxy () const;
		void SetSOCKS5Proxy (const QString&);

		QString GetStunServer () const;
		void SetStunServer (const QString&);

		int GetStunPort () const;
		void SetStunPort (int);

		QString GetTurnServer () const;
		void SetTurnServer (const QString&);

		int GetTurnPort () const;
		void SetTurnPort (int);

		QString GetTurnUser () const;
		void SetTurnUser (const QString&);

		QString GetTurnPassword () const;
		void SetTurnPassword (const QString&);

		QString GetPassword () const;
	private slots:
		void on_UpdatePassword__released ();
	};
}
}
}

#endif
