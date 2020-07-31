/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QDialog>
#include "ui_editurlsdialog.h"
#include "structures.h"

class QStandardItemModel;

namespace LC
{
namespace XProxy
{
	class EditUrlsDialog : public QDialog
	{
		Q_OBJECT

		Ui::EditUrlsDialog Ui_;
		QList<ReqTarget> Items_;

		QStandardItemModel * const Model_;
	public:
		EditUrlsDialog (const QList<ReqTarget>&, QWidget* = nullptr);

		const QList<ReqTarget>& GetTargets () const;
	private slots:
		void on_AddButton__released ();
		void on_EditButton__released ();
		void on_RemoveButton__released ();
	};
}
}
