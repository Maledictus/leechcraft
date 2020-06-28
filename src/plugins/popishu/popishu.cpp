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

#include "popishu.h"
#include <QTranslator>
#include <QIcon>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/iiconthememanager.h>
#include <interfaces/entitytesthandleresult.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/util.h>
#include "editorpage.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace Popishu
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("popishu");

		Proxy_ = proxy;

		TabClass_.TabClass_ = "Popishu";
		TabClass_.VisibleName_ = "Popishu";
		TabClass_.Description_ = tr ("The Popishu text editor");
		TabClass_.Icon_ = GetIcon ();
		TabClass_.Priority_ = 70;
		TabClass_.Features_ = TFOpenableByRequest | TFSuggestOpening;

		XmlSettingsDialog_ = std::make_shared<Util::XmlSettingsDialog> ();
		XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (), "popishusettings.xml");
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Popishu";
	}

	QString Plugin::GetName () const
	{
		return "Popishu";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Plain text editor with syntax highlighting and stuff.");
	}

	QIcon Plugin::GetIcon () const
	{
		return Proxy_->GetIconThemeManager ()->GetPluginIcon ();
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return { TabClass_ };
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "Popishu")
			AnnouncePage (MakeEditorPage ());
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& entity) const
	{
		const bool can = entity.Mime_ == "x-leechcraft/plain-text-document" &&
				entity.Entity_.canConvert<QString> ();

		return can ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity e)
	{
		auto page = MakeEditorPage ();
		page->SetText (e.Entity_.toString ());

		QString language = e.Additional_ ["Language"].toString ();
		bool isTempDocumnet = e.Additional_ ["IsTemporaryDocument"].toBool ();
		if (!language.isEmpty ())
			page->SetLanguage (language);
		page->SetTemporaryDocument (isTempDocumnet);

		AnnouncePage (page);
	}

	std::shared_ptr<Util::XmlSettingsDialog> Plugin::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	void Plugin::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& info : infos)
		{
			const auto page = MakeEditorPage ();
			for (const auto& prop : info.DynProperties_)
				page->setProperty (prop.first, prop.second);

			QDataStream istr { info.Data_ };
			QByteArray tabId;
			istr >> tabId;

			if (tabId == "EditorPage/1")
				page->RestoreState (istr);

			AnnouncePage (page);
		}
	}

	bool Plugin::HasSimilarTab (const QByteArray& data, const QList<QByteArray>& others) const
	{
		return StandardSimilarImpl (data, others,
				[] (const QByteArray& data)
				{
					QByteArray id;
					QString filename;
					QDataStream str { data };
					str >> id >> filename;
					return std::make_tuple (id, filename);
				});
	}

	EditorPage* Plugin::MakeEditorPage ()
	{
		auto result = new EditorPage (Proxy_, TabClass_, this);
		connect (result,
				&EditorPage::removeTab,
				this,
				&Plugin::removeTab);
		connect (result,
				&EditorPage::changeTabName,
				this,
				&Plugin::changeTabName);
		return result;
	}

	void Plugin::AnnouncePage (EditorPage *page)
	{
		emit addNewTab (page->GetTabRecoverName (), page);
		emit raiseTab (page);
		emit changeTabIcon (page, QIcon { "lcicons:/resources/images/popishu.svg" });
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_popishu, LC::Popishu::Plugin);
