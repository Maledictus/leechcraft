/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "glance.h"
#include <QAction>
#include <QTabWidget>
#include <QToolBar>
#include <QMainWindow>
#include <util/util.h>
#include <util/sll/qtutil.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include "glanceshower.h"

namespace LC::Plugins::Glance
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("glance"_qs);

		ActionGlance_ = new QAction (GetName (), this);
		ActionGlance_->setToolTip (tr ("Show the quick overview of tabs"));
		ActionGlance_->setShortcut ("Ctrl+Shift+G"_qs);
		ActionGlance_->setShortcutContext (Qt::ApplicationShortcut);
		ActionGlance_->setProperty ("ActionIcon", "view-list-icons");
		ActionGlance_->setProperty ("Action/ID", GetUniqueID () + "_glance");

		connect (ActionGlance_,
				SIGNAL (triggered ()),
				this,
				SLOT (on_ActionGlance__triggered ()));
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Glance";
	}

	QString Plugin::GetName () const
	{
		return "Glance"_qs;
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Quick overview of tabs.");
	}

	QIcon Plugin::GetIcon () const
	{
		return GetProxyHolder ()->GetIconThemeManager ()->GetPluginIcon ();
	}

	void Plugin::on_ActionGlance__triggered ()
	{
		const auto glance = new GlanceShower;
		auto rootWM = GetProxyHolder ()->GetRootWindowsManager ();
		glance->SetTabWidget (rootWM->GetTabWidget (rootWM->GetPreferredWindowIndex ()));

		connect (glance,
				&GlanceShower::finished,
				ActionGlance_,
				&QAction::setEnabled);

		ActionGlance_->setEnabled (false);
		glance->Start ();
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace aep) const
	{
		QList<QAction*> result;
		if (aep == ActionsEmbedPlace::QuickLaunch)
			result << ActionGlance_;
		return result;
	}

	QMap<QByteArray, ActionInfo> Plugin::GetActionInfo () const
	{
		const auto& iconName = ActionGlance_->property ("ActionIcon").toString ();
		ActionInfo info
		{
			ActionGlance_->text (),
			ActionGlance_->shortcut (),
			GetProxyHolder ()->GetIconThemeManager ()->GetIcon (iconName)
		};
		return { { "ShowList", info } };
	}

	void Plugin::SetShortcut (const QByteArray&, const QKeySequences_t& seqs)
	{
		ActionGlance_->setShortcuts (seqs);
	}
}

LC_EXPORT_PLUGIN (leechcraft_glance, LC::Plugins::Glance::Plugin);
