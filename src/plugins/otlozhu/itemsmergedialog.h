/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QDialog>
#include "ui_itemsmergedialog.h"

namespace LC
{
namespace Otlozhu
{
	class ItemsMergeDialog : public QDialog
	{
		Q_OBJECT

		Ui::ItemsMergeDialog Ui_;
	public:
		enum class Priority
		{
			Imported,
			Current
		};
		enum class SameTitle
		{
			Merge,
			LeaveDistinct
		};

		ItemsMergeDialog (int, QWidget* = 0);

		Priority GetPriority () const;
		SameTitle GetSameTitle () const;
	};
}
}
