/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "albumsettingsdialog.h"
#include <QPushButton>
#include <QtDebug>
#include <util/gui/clearlineeditaddon.h>
#include "fotobilderaccount.h"
#include "selectgroupsdialog.h"

namespace LeechCraft
{
namespace Blasq
{
namespace DeathNote
{
	AlbumSettingsDialog::AlbumSettingsDialog (const QString& name, const QString& login,
			FotoBilderAccount *acc, QWidget *parent)
	: QDialog (parent)
	, PrivacyLevel_ (255)
	, Login_ (login)
	, Account_ (acc)
	{
		Ui_.setupUi (this);
		Ui_.Name_->setText (name);

		new Util::ClearLineEditAddon (Account_->GetProxy (), Ui_.Name_);

		connect (Ui_.Name_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (validate ()));
		validate ();
	}

	QString AlbumSettingsDialog::GetName () const
	{
		return Ui_.Name_->text ();
	}

	int AlbumSettingsDialog::GetPrivacyLevel () const
	{
		return PrivacyLevel_;
	}

	void AlbumSettingsDialog::validate ()
	{
		const bool isValid = !GetName ().isEmpty ();
		Ui_.ButtonBox_->button (QDialogButtonBox::Ok)->setEnabled (isValid);
	}

	void AlbumSettingsDialog::on_PhotosPrivacy__currentIndexChanged (int index)
	{
		switch (index)
		{
		case 0:
			PrivacyLevel_ = 255;
			break;
		case 1:
			PrivacyLevel_ = 254;
			break;
		case 2:
		{
			SelectGroupsDialog dlg (Login_, Account_);
			if (dlg.exec () == QDialog::Rejected)
				break;

			PrivacyLevel_ = dlg.GetSelectedGroupId ();
			break;
		}
		default:
			qWarning () << Q_FUNC_INFO
					<< "unknown index of photo privacy level";
			break;
		}
	}

}
}
}
