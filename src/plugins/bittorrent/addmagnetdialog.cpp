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

#include "addmagnetdialog.h"
#include <QClipboard>
#include <QFileDialog>
#include <QUrl>
#include <QUrlQuery>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include "core.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace BitTorrent
{
	namespace
	{
		bool IsMagnet (const QString& link)
		{
			const auto& url = QUrl::fromUserInput (link);
			if (!url.isValid () || url.scheme () != "magnet")
				return false;

			const auto& items = QUrlQuery { url }.queryItems ();
			return std::any_of (items.begin (), items.end (),
					[] (const auto& item) { return item.first == "xt" && item.second.startsWith ("urn:btih:"); });
		}

		QString CheckClipboard (QClipboard::Mode mode)
		{
			const auto& text = qApp->clipboard ()->text (mode);
			return IsMagnet (text) ? text : QString {};
		}
	}

	AddMagnetDialog::AddMagnetDialog (QWidget *parent)
	: QDialog { parent }
	{
		Ui_.setupUi (this);

		auto text = CheckClipboard (QClipboard::Clipboard);
		if (text.isEmpty ())
			text = CheckClipboard (QClipboard::Selection);

		if (!text.isEmpty ())
			Ui_.Magnet_->setText (text);

		const auto& dir = XmlSettingsManager::Instance ()->
				property ("LastSaveDirectory").toString ();
		Ui_.SavePath_->setText (dir);

		checkComplete ();
		connect (Ui_.SavePath_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (checkComplete ()));
		connect (Ui_.Magnet_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (checkComplete ()));
	}

	QString AddMagnetDialog::GetLink () const
	{
		return Ui_.Magnet_->text ();
	}

	QString AddMagnetDialog::GetPath () const
	{
		return Ui_.SavePath_->text ();
	}

	QStringList AddMagnetDialog::GetTags () const
	{
		auto tm = Core::Instance ()->GetProxy ()->GetTagsManager ();

		QStringList result;
		for (const auto& tag : tm->Split (Ui_.Tags_->text ()))
			result << tm->GetID (tag);
		return result;
	}

	void AddMagnetDialog::on_BrowseButton__released()
	{
		const auto& dir = QFileDialog::getExistingDirectory (this,
				tr ("Select save directory"),
				Ui_.SavePath_->text (),
				{});
		if (dir.isEmpty ())
			return;

		XmlSettingsManager::Instance ()->setProperty ("LastSaveDirectory", dir);
		Ui_.SavePath_->setText (dir);
	}

	void AddMagnetDialog::checkComplete ()
	{
		const auto isComplete = !Ui_.SavePath_->text ().isEmpty () &&
				IsMagnet (Ui_.Magnet_->text ());
		Ui_.ButtonBox_->button (QDialogButtonBox::Ok)->setEnabled (isComplete);
	}
}
}
