/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "messagechangelistener.h"
#include <QStringList>
#include <QtDebug>
#include <vmime/net/folder.hpp>
#include "vmimeconversions.h"

namespace LeechCraft
{
namespace Snails
{
	MessageChangeListener::MessageChangeListener (QObject *parent)
	: QObject { parent }
	{
	}

	std::shared_ptr<void> MessageChangeListener::Disable ()
	{
		if (!IsEnabled_)
			return {};

		IsEnabled_ = false;
		return std::shared_ptr<void> (nullptr, [this] (void*) { IsEnabled_ = true; });
	}

	void MessageChangeListener::messageChanged (const vmime::shared_ptr<vmime::net::events::messageChangedEvent>& event)
	{
		if (!IsEnabled_)
			return;

		const auto& folder = event->getFolder ();

		QList<size_t> numsList;
		for (const auto num : event->getNumbers ())
			numsList << num;

		emit messagesChanged (GetFolderPath (folder), numsList);
	}
}
}
