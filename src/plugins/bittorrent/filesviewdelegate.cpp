/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include <QModelIndex>
#include <QSpinBox>
#include <QLineEdit>
#include <QApplication>
#include <QTreeView>
#include <QtDebug>
#include <util/util.h>
#include <util/gui/util.h>
#include "filesviewdelegate.h"
#include "torrentfilesmodel.h"

namespace LC
{
namespace BitTorrent
{
	FilesViewDelegate::FilesViewDelegate (QTreeView *parent)
	: QStyledItemDelegate (parent)
	, View_ (parent)
	{
	}

	QWidget* FilesViewDelegate::createEditor (QWidget *parent,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
		{
			const auto box = new QSpinBox (parent);
			box->setRange (0, 7);
			return box;
		}
		else if (index.column () == TorrentFilesModel::ColumnPath)
			return new QLineEdit (parent);
		else
			return QStyledItemDelegate::createEditor (parent, option, index);
	}

	void FilesViewDelegate::paint (QPainter *painter,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () != TorrentFilesModel::ColumnProgress)
		{
			QStyledItemDelegate::paint (painter, option, index);
			return;
		}
		else
		{
			QStyleOptionProgressBar progressBarOption;
			progressBarOption.state = QStyle::State_Enabled;
			progressBarOption.direction = QApplication::layoutDirection ();
			progressBarOption.rect = option.rect;
			progressBarOption.fontMetrics = QApplication::fontMetrics ();
			progressBarOption.minimum = 0;
			progressBarOption.maximum = 100;
			progressBarOption.textAlignment = Qt::AlignCenter;
			progressBarOption.textVisible = true;

			double progress = index.data (TorrentFilesModel::RoleProgress).toDouble ();
			qlonglong size = index.data (TorrentFilesModel::RoleSize).toLongLong ();
			qlonglong done = progress * size;
			progressBarOption.progress = progress < 0 ?
					0 :
					static_cast<int> (progress * 100);

			const auto& text = tr ("%1% (%2 of %3)")
					.arg (static_cast<int> (progress * 100))
					.arg (Util::MakePrettySize (done))
					.arg (Util::MakePrettySize (size));
			progressBarOption.text = Util::ElideProgressBarText (text, option);

			QApplication::style ()->drawControl (QStyle::CE_ProgressBar,
					&progressBarOption, painter);
		}
	}

	void FilesViewDelegate::setEditorData (QWidget *editor, const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
			qobject_cast<QSpinBox*> (editor)->
				setValue (index.data (TorrentFilesModel::RolePriority).toInt ());
		else if (index.column () == TorrentFilesModel::ColumnPath)
		{
			const auto& data = index.data (TorrentFilesModel::RoleFullPath);
			qobject_cast<QLineEdit*> (editor)->setText (data.toString ());
		}
		else
			QStyledItemDelegate::setEditorData (editor, index);
	}

	void FilesViewDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
			const QModelIndex& index) const
	{
		if (index.column () == TorrentFilesModel::ColumnPriority)
		{
			int value = qobject_cast<QSpinBox*> (editor)->value ();
			const auto& sindexes = View_->selectionModel ()->selectedRows ();
			for (const auto& selected : sindexes)
				model->setData (index.sibling (selected.row (), index.column ()), value);
		}
		else if (index.column () == TorrentFilesModel::ColumnPath)
		{
			const auto& oldData = index.data (TorrentFilesModel::RoleFullPath);
			const auto& newText = qobject_cast<QLineEdit*> (editor)->text ();
			if (oldData.toString () == newText)
				return;

			model->setData (index, newText);
		}
		else
			QStyledItemDelegate::setModelData (editor, model, index);
	}

	void FilesViewDelegate::updateEditorGeometry (QWidget *editor,
			const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (index.column () != TorrentFilesModel::ColumnPath)
		{
			QStyledItemDelegate::updateEditorGeometry (editor, option, index);
			return;
		}

		auto rect = option.rect;
		rect.setX (0);
		rect.setWidth (editor->parentWidget ()->width ());
		editor->setGeometry (rect);
	}
}
}
