/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef PLUGINS_AZOTH_PROXYOBJECT_H
#define PLUGINS_AZOTH_PROXYOBJECT_H
#include <QObject>
#include <QHash>
#include "interfaces/iproxyobject.h"

namespace LeechCraft
{
namespace Plugins
{
namespace Azoth
{
	class ProxyObject : public QObject
					  , public Plugins::IProxyObject
	{
		Q_OBJECT

		Q_INTERFACES (LeechCraft::Plugins::Azoth::Plugins::IProxyObject)

		QHash<QString, Plugins::AuthStatus> SerializedStr2AuthStatus_;
	public:
		ProxyObject (QObject* = 0);

		QString GetPassword (QObject*);
		void SetPassword (const QString&, QObject*);
		QString GetOSName ();
		QString StateToString (Plugins::State) const;
		QString AuthStatusToString (Plugins::AuthStatus) const;
		Plugins::AuthStatus AuthStatusFromString (const QString&) const;
	};
}
}
}

#endif
