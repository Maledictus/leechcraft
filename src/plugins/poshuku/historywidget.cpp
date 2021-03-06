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

#include "historywidget.h"
#include <QDateTime>
#include <util/compat/fontwidth.h>
#include <util/sll/curry.h>
#include "core.h"
#include "historymodel.h"
#include "historyfiltermodel.h"

namespace LC
{
namespace Poshuku
{
	HistoryWidget::HistoryWidget (QWidget *parent)
	: QWidget { parent }
	, HistoryFilterModel_ { new HistoryFilterModel { this } }
	{
		Ui_.setupUi (this);

		HistoryFilterModel_->setSourceModel (Core::Instance ().GetHistoryModel ());
		HistoryFilterModel_->setDynamicSortFilter (true);
		Ui_.HistoryView_->setModel (HistoryFilterModel_);

		connect (Ui_.HistoryFilterLine_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (updateHistoryFilter ()));
		connect (Ui_.HistoryFilterType_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (updateHistoryFilter ()));
		connect (Ui_.HistoryFilterCaseSensitivity_,
				SIGNAL (stateChanged (int)),
				this,
				SLOT (updateHistoryFilter ()));

		const auto itemsHeader = Ui_.HistoryView_->header ();
		const auto width = Util::Curry (&Util::Compat::Width, fontMetrics ());
		itemsHeader->resizeSection (0,
				width ("Average site title can be very big, it's also the "
					"most important part, so it's priority is the biggest."));
		itemsHeader->resizeSection (1, width (QDateTime::currentDateTime ().toString () + " space"));
		itemsHeader->resizeSection (2, width ("Average URL could be very very long, but we don't account this."));
	}

	void HistoryWidget::on_HistoryView__activated (const QModelIndex& index)
	{
		if (!index.parent ().isValid ())
			return;

		const auto& url = index.sibling (index.row (), HistoryModel::ColumnURL).data ().toString ();
		Core::Instance ().NewURL (url);
	}

	void HistoryWidget::updateHistoryFilter ()
	{
		const int section = Ui_.HistoryFilterType_->currentIndex ();
		const auto& text = Ui_.HistoryFilterLine_->text ();

		switch (section)
		{
		case 1:
			HistoryFilterModel_->setFilterWildcard (text);
			break;
		case 2:
			HistoryFilterModel_->setFilterRegExp (text);
			break;
		default:
			HistoryFilterModel_->setFilterFixedString (text);
			break;
		}

		const auto cs = Ui_.HistoryFilterCaseSensitivity_->checkState () == Qt::Checked ?
				Qt::CaseSensitive :
				Qt::CaseInsensitive;
		HistoryFilterModel_->setFilterCaseSensitivity (cs);
	}
}
}
