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

#include "newnickservidentifydialog.h"
#include <QMessageBox>

namespace LC
{
namespace Azoth
{
namespace Acetamide
{
	NewNickServIdentifyDialog::NewNickServIdentifyDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);
	}

	QString NewNickServIdentifyDialog::GetServer () const
	{
		return Ui_.Server_->text ();
	}

	QString NewNickServIdentifyDialog::GetNickName () const
	{
		return Ui_.NickName_->text ();
	}

	QString NewNickServIdentifyDialog::GetNickServNickName () const
	{
		return Ui_.NickServNickname_->text ();
	}

	QString NewNickServIdentifyDialog::GetAuthString () const
	{
		return Ui_.NickServAuthString_->text ();
	}

	QString NewNickServIdentifyDialog::GetAuthMessage () const
	{
		return Ui_.AuthMessage_->text ();
	}

	void NewNickServIdentifyDialog::accept ()
	{
		if (GetServer ().isEmpty () ||
				GetNickName ().isEmpty () ||
				GetNickServNickName ().isEmpty () ||
				GetAuthString ().isEmpty () ||
				GetAuthMessage ().isEmpty ())
			return;

		QDialog::accept ();
	}

	void NewNickServIdentifyDialog::SetServer (const QString& server)
	{
		Ui_.Server_->setText (server);
	}

	void NewNickServIdentifyDialog::SetNickName (const QString& nick)
	{
		Ui_.NickName_->setText (nick);
	}

	void NewNickServIdentifyDialog::SetNickServNickName (const QString& nickServ)
	{
		Ui_.NickServNickname_->setText (nickServ);
	}

	void NewNickServIdentifyDialog::SetAuthString (const QString& authString)
	{
		Ui_.NickServAuthString_->setText (authString);
	}

	void NewNickServIdentifyDialog::SetAuthMessage (const QString& authMessage)
	{
		Ui_.AuthMessage_->setText (authMessage);
	}
}
}
}
