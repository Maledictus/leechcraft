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

#include "tasks.h"
#include <QAbstractItemModel>
#include <util/sll/prelude.h>
#include <interfaces/ijobholder.h>
#include <interfaces/iinfo.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include "core.h"

namespace LeechCraft
{
namespace DBusManager
{
	QStringList Tasks::GetHolders () const
	{
		return Util::Map (Core::Instance ().GetProxy ()->GetPluginsManager ()->GetAllCastableRoots<IJobHolder*> (),
				[] (auto plugin) { return qobject_cast<IInfo*> (plugin)->GetName (); });
	}

	Tasks::RowCountResult_t Tasks::RowCount (const QString& name) const
	{
		for (auto plugin : Core::Instance ().GetProxy ()->GetPluginsManager ()->GetAllCastableRoots<IJobHolder*> ())
		{
			if (qobject_cast<IInfo*> (plugin)->GetName () != name)
				continue;

			return RowCountResult_t::Right (qobject_cast<IJobHolder*> (plugin)->GetRepresentation ()->rowCount ());
		}

		return RowCountResult_t::Left (IdentifierNotFound { name });
	}

	Tasks::GetDataResult_t Tasks::GetData (const QString& name, int r, int role) const
	{
		for (auto plugin : Core::Instance ().GetProxy ()->GetPluginsManager ()->GetAllCastableRoots<IJobHolder*> ())
		{
			if (qobject_cast<IInfo*> (plugin)->GetName () != name)
				continue;

			const auto model = qobject_cast<IJobHolder*> (plugin)->GetRepresentation ();

			QVariantList result;
			for (int i = 0, size = model->columnCount (); i < size; ++i)
				result << model->index (r, i).data (role);
			return GetDataResult_t::Right (result);
		}

		return GetDataResult_t::Left (IdentifierNotFound { name });
	}
}
}
