/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QWidget>
#include "ui_accregfirstpage.h"

namespace LC
{
namespace Azoth
{
namespace VelvetBird
{
	class AccRegFirstPage : public QWidget
	{
		Ui::AccRegFirstPage Ui_;
	public:
		AccRegFirstPage (QWidget* = 0);

		QString GetName () const;
		QString GetNick () const;
	};
}
}
}
