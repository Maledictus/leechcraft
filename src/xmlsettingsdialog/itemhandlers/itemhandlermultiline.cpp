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

#include "itemhandlermultiline.h"
#include <QLabel>
#include <QTextEdit>
#include <QGridLayout>
#include <QApplication>
#include <QtDebug>
#include <util/compat/fontwidth.h>

namespace LC
{
	bool ItemHandlerMultiLine::CanHandle (const QDomElement& element) const
	{
		return element.attribute ("type") == "multiline";
	}

	void ItemHandlerMultiLine::Handle (const QDomElement& item,
			QWidget *pwidget)
	{
		QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
		QLabel *label = new QLabel (XSD_->GetLabel (item));
		label->setWordWrap (false);

		const QVariant& value = XSD_->GetValue (item);

		QTextEdit *edit = new QTextEdit ();
		XSD_->SetTooltip (edit, item);
		edit->setPlainText (value.toStringList ().join ("\n"));
		edit->setObjectName (item.attribute ("property"));
		edit->setMinimumWidth (Util::Compat::Width (QApplication::fontMetrics (), "thisismaybeadefaultsetting"));
		connect (edit,
				SIGNAL (textChanged ()),
				this,
				SLOT (updatePreferences ()));

		edit->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
		edit->setProperty ("SearchTerms", label->text ());

		int row = lay->rowCount ();
		QString pos = item.attribute ("position");
		if (pos == "bottom")
		{
			lay->addWidget (label, row, 0, Qt::AlignLeft);
			lay->addWidget (edit, row + 1, 0);
		}
		else if (pos == "right")
		{
			lay->addWidget (label, row, 0, Qt::AlignRight | Qt::AlignTop);
			lay->addWidget (edit, row, 1);
		}
		else if (pos == "left")
		{
			lay->addWidget (label, row, 1, Qt::AlignLeft | Qt::AlignTop);
			lay->addWidget (edit, row, 0);
		}
		else if (pos == "top")
		{
			lay->addWidget (edit, row, 0);
			lay->addWidget (label, row + 1, 0, Qt::AlignLeft);
		}
		else
		{
			lay->addWidget (label, row, 0, Qt::AlignRight | Qt::AlignTop);
			lay->addWidget (edit, row, 1);
		}

		lay->setContentsMargins (2, 2, 2, 2);
	}

	void ItemHandlerMultiLine::SetValue (QWidget *widget, const QVariant& value) const
	{
		QTextEdit *edit = qobject_cast<QTextEdit*> (widget);
		if (!edit)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QTextEdit"
				<< widget;
			return;
		}
		edit->setPlainText (value.toStringList ().join ("\n"));
	}

	QVariant ItemHandlerMultiLine::GetObjectValue (QObject *object) const
	{
		QTextEdit *edit = qobject_cast<QTextEdit*> (object);
		if (!edit)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QTextEdit"
				<< object;
			return QVariant ();
		}
		return edit->toPlainText ().split ('\n', QString::SkipEmptyParts);
	}
}
