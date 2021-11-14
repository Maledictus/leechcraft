/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>

namespace LC::Azoth::Acetamide
{
	struct IrcMessageOptions;

	class IrcErrorHandler : public QObject
	{
	public:
		explicit IrcErrorHandler (QObject *parent);

		void HandleError (const IrcMessageOptions& opts);
		bool IsError (int code);
	};
}
