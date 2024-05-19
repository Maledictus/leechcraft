/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>
#include "monocleutilconfig.h"

namespace LC::Monocle
{
	class MONOCLE_UTIL_API DocumentSignals : public QObject
	{
		Q_OBJECT
	public:
		using QObject::QObject;
	signals:
		/** @brief Emitted when printing is requested.
		 *
		 * This signal is emitted when printing is requested, for
		 * example, by a link action.
		 *
		 * @param[out] pages The list of pages to print, or an empty list
		 * to print all pages.
		 */
		void printRequested (const QList<int>& pages);
	};
}
