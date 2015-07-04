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

#include "fatape.h"
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QNetworkRequest>
#include <QProcess>
#include <QStringList>
#include <QTranslator>
#include <util/util.h>
#include <util/sys/paths.h>
#include <interfaces/core/icoreproxy.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "userscriptsmanagerwidget.h"
#include "userscriptinstallerdialog.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace FatApe
{
	template<typename Iter, typename Pred, typename Func>
	void apply_if (Iter first, Iter last, Pred pred, Func func)
	{
		for (; first != last; ++first)
			if (pred (*first))
				func (*first);
	}

	void WrapText (QString& text, int width = 80)
	{
		int curWidth = width;

		while (curWidth < text.length ())
		{
			int spacePos = text.lastIndexOf (' ', curWidth);

			if (spacePos == -1)
				spacePos = text.indexOf (' ', curWidth);
			if (spacePos != -1)
			{
				text [spacePos] = '\n';
				curWidth = spacePos + width + 1;
			}
		}
	}

	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		CoreProxy_ = proxy;

		Util::InstallTranslator ("poshuku_fatape");

		QDir scriptsDir (Util::CreateIfNotExists ("data/poshuku/fatape/scripts"));

		if (!scriptsDir.exists ())
			return;

		QStringList filter ("*.user.js");

		for (const auto& script : scriptsDir.entryList (filter, QDir::Files))
			UserScripts_.append (scriptsDir.absoluteFilePath (script));

		Model_ = std::make_shared<QStandardItemModel> ();
		Model_->setHorizontalHeaderLabels ({ tr ("Name"), tr ("Description") });
		for (const auto& script : UserScripts_)
			AddScriptToManager (script);

		connect (Model_.get (),
				SIGNAL (itemChanged (QStandardItem*)),
				this,
				SLOT (handleItemChanged (QStandardItem*)));

		SettingsDialog_.reset (new Util::XmlSettingsDialog);
		SettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
				"poshukufatapesettings.xml");
		SettingsDialog_->SetCustomWidget ("UserScriptsManagerWidget",
				new UserScriptsManagerWidget (Model_.get (), this));
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.FatApe";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku FatApe";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("GreaseMonkey support layer for the Poshuku browser.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/poshuku/plugins/fatape/resources/images/fatape.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	void Plugin::hookInitialLayoutCompleted (LeechCraft::IHookProxy_ptr,
			QWebPage*, QWebFrame *frame)
	{
		auto match = [frame] (const UserScript& us) { return us.MatchToPage (frame->url ().toString ()); };
		auto inject = [frame, this] (const UserScript& us) { us.Inject (frame, Proxy_); };

		apply_if (UserScripts_.begin (), UserScripts_.end (), match, inject);
	}

	void Plugin::initPlugin (QObject *proxy)
	{
		Proxy_ = qobject_cast<IProxyObject*>(proxy);

		if (!Proxy_)
		{
			qWarning () << Q_FUNC_INFO
				<< "unable to cast"
				<< proxy
				<< "to IProxyObject";
		}
	}

	void Plugin::EditScript (int scriptIndex)
	{
		const UserScript& script = UserScripts_.at (scriptIndex);
		QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_Poshuku_FatApe");
		const QString& editor = settings.value ("editor").toString ();

		if (editor.isEmpty ())
			return;

		QProcess::execute (editor, QStringList (script.Path ()));
	}

	void Plugin::DeleteScript (int scriptIndex)
	{
		UserScripts_ [scriptIndex].Delete ();
		UserScripts_.removeAt (scriptIndex);
	}

	void Plugin::SetScriptEnabled (int scriptIndex, bool value)
	{
		UserScripts_ [scriptIndex].SetEnabled (value);
	}

	void Plugin::hookAcceptNavigationRequest (LeechCraft::IHookProxy_ptr proxy, QWebPage*,
			QWebFrame*, QNetworkRequest request, QWebPage::NavigationType)
	{
		if (!request.url ().path ().endsWith ("user.js", Qt::CaseInsensitive) ||
				request.url ().scheme () == "file")
			return;

		UserScriptInstallerDialog installer (this,
				CoreProxy_->GetNetworkAccessManager (), request.url ());

		switch (installer.exec ())
		{
		case UserScriptInstallerDialog::Install:
			UserScripts_.append (UserScript (installer.TempScriptPath ()));
			UserScripts_.last ().Install (CoreProxy_->GetNetworkAccessManager ());
			AddScriptToManager (UserScripts_.last ());
			break;
		case UserScriptInstallerDialog::ShowSource:
			Proxy_->OpenInNewTab (QUrl::fromLocalFile (installer.TempScriptPath ()));
			break;
		case UserScriptInstallerDialog::Cancel:
			QFile::remove (installer.TempScriptPath ());
			break;
		default:
			break;
		}

		proxy->CancelDefault ();
	}

	void Plugin::AddScriptToManager (const UserScript& script)
	{
		QString scriptDesc = script.Description ();
		QStandardItem* name = new QStandardItem (script.Name ());
		QStandardItem* description = new QStandardItem (scriptDesc);

		name->setCheckState (script.IsEnabled () ? Qt::Checked : Qt::Unchecked);
		name->setEditable (false);
		name->setCheckable (true);
		name->setData (script.IsEnabled (), EnabledRole);
		description->setEditable (false);
		WrapText (scriptDesc);
		description->setToolTip (scriptDesc);
		description->setData (script.IsEnabled (), EnabledRole);

		Model_->appendRow ({ name, description });
	}

	void Plugin::handleItemChanged (QStandardItem *item)
	{
		if (item->column ())
			return;

		auto& script = UserScripts_ [item->row ()];
		const auto shouldEnable = item->checkState () == Qt::Checked;
		if (shouldEnable != script.IsEnabled ())
			script.SetEnabled (shouldEnable);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_fatape, LeechCraft::Poshuku::FatApe::Plugin);
