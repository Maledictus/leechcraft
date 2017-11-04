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

#include "newlife.h"
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QTranslator>
#include <util/util.h>
#include "common/imimportpage.h"
#include "importwizard.h"

namespace LeechCraft
{
namespace NewLife
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("newlife");

		Proxy_ = proxy;

		Common::IMImportPage::SetPluginInstance (this);

		ImporterAction_ = new QAction (tr ("Import settings..."), this);
		ImporterAction_->setProperty ("ActionIcon", "document-import");
		connect (ImporterAction_,
				SIGNAL (triggered ()),
				this,
				SLOT (runWizard ()));
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.NewLife";
	}

	QString Plugin::GetName () const
	{
		return "New Life";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("The settings importer.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/newlife.svg");
		return icon;
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace place) const
	{
		QList<QAction*> result;

		if (place == ActionsEmbedPlace::ToolsMenu)
			result << ImporterAction_;

		return result;
	}

	void Plugin::runWizard ()
	{
		ImportWizard *wiz = new ImportWizard (Proxy_, this);
		connect (wiz,
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		wiz->show ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_newlife, LeechCraft::NewLife::Plugin);
