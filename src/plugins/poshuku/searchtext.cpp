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

#include "searchtext.h"
#include <util/xpc/util.h>
#include <interfaces/core/icoreproxy.h>
#include "core.h"

namespace LeechCraft
{
namespace Poshuku
{
	SearchText::SearchText (const QString& text, QWidget *parent)
	: QDialog (parent)
	, Text_ (text)
	{
		Ui_.setupUi (this);
		Ui_.Label_->setText (tr ("Search %1 with:").arg ("<em>" + text + "</em>"));

		for (const auto& cat : Core::Instance ().GetProxy ()->GetSearchCategories ())
			new QTreeWidgetItem (Ui_.Tree_, QStringList { cat });

		on_MarkAll__released ();

		connect (this,
				&QDialog::accepted,
				this,
				&SearchText::DoSearch);
	}

	void SearchText::DoSearch ()
	{
		QStringList selected;
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			if (Ui_.Tree_->topLevelItem (i)->checkState (0) == Qt::Checked)
				selected << Ui_.Tree_->topLevelItem (i)->text (0);
		if (selected.isEmpty ())
			return;

		auto e = Util::MakeEntity (Text_,
				QString (),
				FromUserInitiated,
				"x-leechcraft/category-search-request");

		e.Additional_ ["Categories"] = selected;

		emit gotEntity (e);
	}

	void SearchText::on_MarkAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Checked);
	}

	void SearchText::on_UnmarkAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Unchecked);
	}
}
}
