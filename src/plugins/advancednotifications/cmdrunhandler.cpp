/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "cmdrunhandler.h"
#include <QProcess>
#include "notificationrule.h"
#include "fields.h"

namespace LC
{
namespace AdvancedNotifications
{
	CmdRunHandler::CmdRunHandler ()
	{
	}

	NotificationMethod CmdRunHandler::GetHandlerMethod () const
	{
		return NMCommand;
	}

	void CmdRunHandler::Handle (const Entity& e, const NotificationRule& rule)
	{
		if (e.Additional_ [Fields::EventCategory].toString () == Fields::Values::CancelEvent)
			return;

		const CmdParams& params = rule.GetCmdParams ();
		if (params.Cmd_.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "empty command";
			return;
		}

		auto args = params.Args_;
		for (auto& arg : args)
		{
			if (arg [0] != '$')
				continue;

			auto stripped = arg;
			stripped.remove (0, 1);
			if (!e.Additional_.contains (stripped))
				continue;

			arg = e.Additional_ [stripped].toString ();
		}

		QProcess::startDetached (params.Cmd_, args);
	}
}
}
