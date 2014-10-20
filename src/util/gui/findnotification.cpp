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

#include "findnotification.h"
#include <QShortcut>
#include "clearlineeditaddon.h"
#include "ui_findnotification.h"

namespace LeechCraft
{
namespace Util
{
	FindNotification::FindNotification (ICoreProxy_ptr proxy, QWidget *parent)
	: Util::PageNotification (parent)
	, Ui_ (new Ui::FindNotification)
	, EscShortcut_ (new QShortcut (QString ("Esc"), this, SLOT (reject ())))
	{
		Ui_->setupUi (this);

		setFocusProxy (Ui_->Pattern_);

		EscShortcut_->setContext (Qt::WidgetWithChildrenShortcut);

		const auto addon = new Util::ClearLineEditAddon (proxy, Ui_->Pattern_);
		addon->SetEscClearsEdit (false);
	}

	FindNotification::~FindNotification ()
	{
		delete Ui_;
	}

	void FindNotification::SetEscCloses (bool close)
	{
		EscShortcut_->setEnabled (close);
	}

	void FindNotification::SetText (const QString& text)
	{
		Ui_->Pattern_->setText (text);
	}

	QString FindNotification::GetText () const
	{
		return Ui_->Pattern_->text ();
	}

	void FindNotification::SetSuccessful (bool success)
	{
		QString ss = QString ("QLineEdit {"
				"background-color:rgb(");
		if (!success)
			ss.append ("255,0,0");
		else
		{
			auto color = QApplication::palette ().color (QPalette::Base);
			color.setRedF (color.redF () / 2);
			color.setBlueF (color.blueF () / 2);

			int r = 0, g = 0, b = 0;
			color.getRgb (&r, &g, &b);

			ss.append (QString ("%1,%2,%3")
					.arg (r)
					.arg (g)
					.arg (b));
		}
		ss.append (") }");
		Ui_->Pattern_->setStyleSheet (ss);
	}

	auto FindNotification::GetFlags () const -> FindFlags
	{
		FindFlags flags;
		if (Ui_->MatchCase_->checkState () == Qt::Checked)
			flags |= FindCaseSensitively;
		if (Ui_->WrapAround_->checkState () == Qt::Checked)
			flags |= FindWrapsAround;
		return flags;
	}

	void FindNotification::findNext ()
	{
		const auto& text = GetText ();
		if (text.isEmpty ())
			return;

		handleNext (text, GetFlags ());
	}

	void FindNotification::findPrevious ()
	{
		const auto& text = GetText ();
		if (text.isEmpty ())
			return;

		handleNext (text, GetFlags () | FindBackwards);
	}

	void FindNotification::clear ()
	{
		SetText ({});
	}

	void FindNotification::reject ()
	{
		Ui_->Pattern_->clear ();
		hide ();
	}

	void FindNotification::on_Pattern__textChanged (const QString& newText)
	{
		Ui_->FindButton_->setEnabled (!newText.isEmpty ());
	}

	void FindNotification::on_FindButton__released ()
	{
		auto flags = GetFlags ();
		if (Ui_->SearchBackwards_->checkState () == Qt::Checked)
			flags |= FindBackwards;

		handleNext (Ui_->Pattern_->text (), flags);
	}
}
}
