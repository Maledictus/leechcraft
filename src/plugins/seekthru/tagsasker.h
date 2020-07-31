/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QDialog>
#include "ui_tagsasker.h"

namespace LC::SeekThru
{
	class TagsAsker : public QDialog
	{
		Ui::TagsAsker Ui_;
	public:
		explicit TagsAsker (const QString&, QWidget* = nullptr);

		QString GetTags () const;
	};
}
