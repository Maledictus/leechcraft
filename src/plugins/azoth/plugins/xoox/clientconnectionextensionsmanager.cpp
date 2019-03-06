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

#include "clientconnectionextensionsmanager.h"
#include <QXmppClient.h>
#include <QXmppArchiveManager.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppEntityTimeManager.h>
#include "adhoccommandmanager.h"
#include "jabbersearchmanager.h"
#include "lastactivitymanager.h"
#include "pingmanager.h"

namespace LeechCraft::Azoth::Xoox
{
	namespace
	{
		auto MakeDefaultExtensions (QXmppClient& client)
		{
			return std::apply ([&client] (auto... types)
					{
						return DefaultExtensions { client.findExtension<std::remove_pointer_t<decltype (types)>> ()... };
					},
					DefaultExtensions {});
		}

		template<typename T>
		T* MakeForType (ClientConnection& conn)
		{
			if constexpr (std::is_constructible_v<T, ClientConnection*>)
				return new T { &conn };
			else
				return new T {};
		}

		auto MakeSimpleExtensions (ClientConnection& conn)
		{
			return std::apply ([&conn] (auto... types)
					{
						return SimpleExtensions { MakeForType<std::remove_pointer_t<decltype (types)>> (conn)... };
					},
					SimpleExtensions {});
		}
	}

	ClientConnectionExtensionsManager::ClientConnectionExtensionsManager (ClientConnection& conn,
			QXmppClient& client, QObject *parent)
	: QObject { parent }
	, DefaultExtensions_ { MakeDefaultExtensions (client) }
	, SimpleExtensions_ { MakeSimpleExtensions (conn) }
	{
		std::apply ([&client] (auto... exts) { (client.addExtension (exts), ...); },
				SimpleExtensions_);
	}
}
