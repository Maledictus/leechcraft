/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include "todoitem.h"

namespace LC
{
namespace Otlozhu
{
	class ICalGenerator
	{
		QList<TodoItem_ptr> Items_;
	public:
		ICalGenerator& operator<< (TodoItem_ptr);
		ICalGenerator& operator<< (QList<TodoItem_ptr>);

		QByteArray operator() () const;
	};
}
}
