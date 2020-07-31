/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>

class ISyncable;

namespace LC
{
namespace Syncer
{
	class SyncableManager : public QObject
	{
		Q_OBJECT
	public:
		SyncableManager (QObject* = 0);

		void AddPlugin (ISyncable*);
	};
}
}
