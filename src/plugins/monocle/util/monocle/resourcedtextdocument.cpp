/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "resourcedtextdocument.h"

namespace LC::Monocle
{
	ResourcedTextDocument::ResourcedTextDocument (const LazyImages_t& images)
	: Images_ { images }
	{
	}

	QVariant ResourcedTextDocument::loadResource (int type, const QUrl& name)
	{
		if (type != QTextDocument::ImageResource)
			return QTextDocument::loadResource (type, name);

		const auto& image = Images_.value (name);
		if (!image.Load_)
			return QTextDocument::loadResource (type, name);

		return image.Load_ (image.NativeSize_);
	}
}