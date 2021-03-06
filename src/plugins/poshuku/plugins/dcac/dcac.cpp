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

#include "dcac.h"
#include <QIcon>
#include <QMenu>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/poshuku/ibrowserwidget.h>
#include <interfaces/poshuku/iwebview.h>
#include "viewsmanager.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace Poshuku
{
namespace DCAC
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("poshuku_dcac");
		ViewsManager_ = new ViewsManager { proxy->GetPluginsManager () };

		XSD_ = std::make_shared<Util::XmlSettingsDialog> ();
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "poshukudcacsettings.xml");
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
		delete ViewsManager_;
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.DCAC";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku DC/AC";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Inverts the colors of web pages.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	void Plugin::hookBrowserWidgetInitialized (IHookProxy_ptr,
			QObject *browser)
	{
		const auto webView = qobject_cast<IBrowserWidget*> (browser)->GetWebView ();
		ViewsManager_->AddView (webView->GetQWidget ());
	}

	void Plugin::hookWebViewContextMenu (IHookProxy_ptr,
			IWebView *view, const ContextMenuInfo&,
			QMenu *menu, WebViewCtxMenuStage menuBuildStage)
	{
		if (menuBuildStage == WVSAfterFinish)
			menu->addAction (ViewsManager_->GetEnableAction (view->GetQWidget ()));
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_dcac, LC::Poshuku::DCAC::Plugin);

