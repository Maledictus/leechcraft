/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include "pasteservicebase.h"

class QNetworkAccessManager;

namespace LC::Azoth::Autopaste
{
	class CodepadService final : public PasteServiceBase
	{
	public:
		using PasteServiceBase::PasteServiceBase;

		void Paste (const PasteParams&);
	};
}
