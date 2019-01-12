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

#include <QMessageBox>
#include <QtDebug>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QCompleter>
#include <QPainter>
#include <QMenu>
#include <QToolBar>
#include <QQueue>
#include <QTimer>
#include <QTranslator>
#include <QCursor>
#include <QKeyEvent>
#include <QInputDialog>
#include <interfaces/entitytesthandleresult.h>
#include <interfaces/core/icoreproxy.h>
#include <util/tags/tagscompletionmodel.h>
#include <util/util.h>
#include <util/tags/categoryselector.h>
#include <util/db/backendselector.h>
#include <util/models/flattofoldersproxymodel.h>
#include <util/shortcuts/shortcutmanager.h>
#include <util/gui/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "itemsfiltermodel.h"
#include "channelsfiltermodel.h"
#include "aggregator.h"
#include "core.h"
#include "addfeeddialog.h"
#include "itemsfiltermodel.h"
#include "channelsfiltermodel.h"
#include "xmlsettingsmanager.h"
#include "importbinary.h"
#include "feedsettings.h"
#include "wizardgenerator.h"
#include "export2fb2dialog.h"
#include "channelsmodel.h"
#include "aggregatortab.h"
#include "storagebackendmanager.h"
#include "exportutils.h"
#include "actionsstructs.h"
#include "representationmanager.h"
#include "dbupdatethread.h"
#include "dbupdatethreadworker.h"
#include "pluginmanager.h"

namespace LeechCraft
{
namespace Aggregator
{
	void Aggregator::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		Util::InstallTranslator ("aggregator");

		qRegisterMetaType<IDType_t> ("IDType_t");
		qRegisterMetaType<QList<IDType_t>> ("QList<IDType_t>");
		qRegisterMetaType<QSet<IDType_t>> ("QSet<IDType_t>");
		qRegisterMetaType<QItemSelection> ("QItemSelection");
		qRegisterMetaType<Item> ("Item");
		qRegisterMetaType<ChannelShort> ("ChannelShort");
		qRegisterMetaType<Channel> ("Channel");
		qRegisterMetaType<channels_container_t> ("channels_container_t");
		qRegisterMetaTypeStreamOperators<Feed> ("LeechCraft::Plugins::Aggregator::Feed");

		TabInfo_ = TabClassInfo
		{
			"Aggregator",
			GetName (),
			GetInfo (),
			GetIcon (),
			0,
			TFSingle | TFOpenableByRequest
		};

		ShortcutMgr_ = new Util::ShortcutManager (proxy, this);

		ChannelActions_ = std::make_shared<ChannelActions> (ShortcutMgr_, this);
		AppWideActions_ = std::make_shared<AppWideActions> (ShortcutMgr_, this);

		ToolMenu_ = AppWideActions_->CreateToolMenu ();
		ToolMenu_->setIcon (GetIcon ());

		Core::Instance ().SetProxy (proxy);

		XmlSettingsDialog_ = std::make_shared<Util::XmlSettingsDialog> ();
		XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (), "aggregatorsettings.xml");
		XmlSettingsDialog_->SetCustomWidget ("BackendSelector",
				new Util::BackendSelector (XmlSettingsManager::Instance ()));

		if (!Core::Instance ().DoDelayedInit ())
		{
			AppWideActions_->SetEnabled (false);
			InitFailed_ = true;
			qWarning () << Q_FUNC_INFO
					<< "core initialization failed";

			auto box = new QMessageBox (QMessageBox::Critical,
					"LeechCraft",
					tr ("Aggregator failed to initialize properly. Check logs and talk with "
						"the developers. Or, at least, check the storage backend settings and "
						"restart LeechCraft.<br /><br />If you are using SQLite backend (the "
						"default), make sure you have the corresponding Qt driver installed."),
					QMessageBox::Ok);
			box->open ();
		}

		connect (AppWideActions_->ActionUpdateFeeds_,
				&QAction::triggered,
				&Core::Instance (),
				&Core::updateFeeds);

		QMetaObject::connectSlotsByName (this);

		ChannelsModel_ = std::make_shared<ChannelsModel> (Proxy_->GetTagsManager ());

		PluginManager_ = std::make_shared<PluginManager> (ChannelsModel_.get ());
		PluginManager_->RegisterHookable (&Core::Instance ());
		PluginManager_->RegisterHookable (&StorageBackendManager::Instance ());
	}

	void Aggregator::SecondInit ()
	{
		if (InitFailed_)
			return;

		ReprManager_ = std::make_shared<RepresentationManager> (RepresentationManager::InitParams {
					ShortcutMgr_,
					*AppWideActions_,
					*ChannelActions_,
					ChannelsModel_.get ()
				});
	}

	void Aggregator::Release ()
	{
		PluginManager_.reset ();
		ReprManager_.reset ();
		AggregatorTab_.reset ();
		ChannelsModel_.reset ();
		Core::Instance ().Release ();
		StorageBackendManager::Instance ().Release ();
	}

	QByteArray Aggregator::GetUniqueID () const
	{
		return "org.LeechCraft.Aggregator";
	}

	QString Aggregator::GetName () const
	{
		return "Aggregator";
	}

	QString Aggregator::GetInfo () const
	{
		return tr ("RSS/Atom feed reader.");
	}

	QStringList Aggregator::Provides () const
	{
		return { "rss" };
	}

	QStringList Aggregator::Needs () const
	{
		return { "http" };
	}

	QStringList Aggregator::Uses () const
	{
		return { "webbrowser" };
	}

	QIcon Aggregator::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/aggregator.svg");
		return icon;
	}

	TabClasses_t Aggregator::GetTabClasses () const
	{
		return { TabInfo_ };
	}

	void Aggregator::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "Aggregator")
		{
			if (!AggregatorTab_)
			{
				AggregatorTab_ = std::make_unique<AggregatorTab> (AggregatorTab::InitParams {
							*AppWideActions_,
							ChannelActions_,
							TabInfo_,
							ShortcutMgr_,
							ChannelsModel_.get (),
							Proxy_->GetTagsManager ()
						},
						this);
				connect (AggregatorTab_.get (),
						&AggregatorTab::removeTabRequested,
						[this] { emit removeTab (AggregatorTab_.get ()); });
			}
			emit addNewTab (AggregatorTab_->GetTabClassInfo ().VisibleName_, AggregatorTab_.get ());

		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	Util::XmlSettingsDialog_ptr Aggregator::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	QAbstractItemModel* Aggregator::GetRepresentation () const
	{
		return ReprManager_->GetRepresentation ();
	}

	void Aggregator::handleTasksTreeSelectionCurrentRowChanged (const QModelIndex& index, const QModelIndex&)
	{
		ReprManager_->HandleRowChanged (Proxy_->MapToSource (index));
	}

	EntityTestHandleResult Aggregator::CouldHandle (const Entity& e) const
	{
		EntityTestHandleResult r;
		if (Core::Instance ().CouldHandle (e))
			r.HandlePriority_ = EntityTestHandleResult::PIdeal;
		return r;
	}

	void Aggregator::Handle (Entity e)
	{
		Core::Instance ().Handle (e);
	}

	void Aggregator::SetShortcut (const QString& name, const QKeySequences_t& shortcuts)
	{
		ShortcutMgr_->SetShortcut (name, shortcuts);
	}

	QMap<QString, ActionInfo> Aggregator::GetActionInfo () const
	{
		return ShortcutMgr_->GetActionInfo ();
	}

	QList<QWizardPage*> Aggregator::GetWizardPages () const
	{
		return CreateWizardPages ();
	}

	QList<QAction*> Aggregator::GetActions (ActionsEmbedPlace place) const
	{
		QList<QAction*> result;

		switch (place)
		{
		case ActionsEmbedPlace::ToolsMenu:
			result << ToolMenu_->menuAction ();
			break;
		case ActionsEmbedPlace::CommonContextMenu:
			result << AppWideActions_->ActionAddFeed_;
			result << AppWideActions_->ActionUpdateFeeds_;
			break;
		case ActionsEmbedPlace::TrayMenu:
			result << AppWideActions_->ActionMarkAllAsRead_;
			result << AppWideActions_->ActionAddFeed_;
			result << AppWideActions_->ActionUpdateFeeds_;
			break;
		default:
			break;
		}

		return result;
	}

	QSet<QByteArray> Aggregator::GetExpectedPluginClasses () const
	{
		return { "org.LeechCraft.Aggregator.GeneralPlugin/1.0" };
	}

	void Aggregator::AddPlugin (QObject *plugin)
	{
		PluginManager_->AddPlugin (plugin);
	}

	void Aggregator::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& recInfo : infos)
		{
			if (recInfo.Data_ == "aggregatortab")
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);

				TabOpenRequested (TabInfo_.TabClass_);
			}
			else
				qWarning () << Q_FUNC_INFO
						<< "unknown context"
						<< recInfo.Data_;
		}
	}

	bool Aggregator::HasSimilarTab (const QByteArray&, const QList<QByteArray>&) const
	{
		return true;
	}

	QModelIndex Aggregator::GetRelevantIndex () const
	{
		if (const auto idx = ReprManager_->GetRelevantIndex ())
			return *idx;
		else
			return AggregatorTab_->GetRelevantIndex ();
	}

	QList<QModelIndex> Aggregator::GetRelevantIndexes () const
	{
		if (const auto idx = ReprManager_->GetRelevantIndex ())
			return { *idx };
		else
			return AggregatorTab_->GetRelevantIndexes ();
	}

	namespace
	{
		void MarkChannel (const QModelIndex& idx, bool unread)
		{
			const auto cid = idx.data (ChannelRoles::ChannelID).value<IDType_t> ();
			auto& dbUpThread = Core::Instance ().GetDBUpdateThread ();
			dbUpThread.ScheduleImpl (&DBUpdateThreadWorker::toggleChannelUnread, cid, unread);
		}
	}

	void Aggregator::on_ActionMarkAllAsRead__triggered ()
	{
		if (XmlSettingsManager::Instance ()->property ("ConfirmMarkAllAsRead").toBool ())
		{
			QMessageBox mbox (QMessageBox::Question,
					"LeechCraft",
					tr ("Do you really want to mark all channels as read?"),
					QMessageBox::Yes | QMessageBox::No,
					nullptr);
			mbox.setDefaultButton (QMessageBox::No);

			QPushButton always (tr ("Always"));
			mbox.addButton (&always, QMessageBox::AcceptRole);

			if (mbox.exec () == QMessageBox::No)
				return;
			else if (mbox.clickedButton () == &always)
				XmlSettingsManager::Instance ()->setProperty ("ConfirmMarkAllAsRead", false);
		}

		for (int i = 0; i < ChannelsModel_->rowCount (); ++i)
			MarkChannel (ChannelsModel_->index (i, 0), false);
	}

	void Aggregator::on_ActionAddFeed__triggered ()
	{
		AddFeedDialog af { Proxy_->GetTagsManager () };
		if (af.exec () == QDialog::Accepted)
			Core::Instance ().AddFeed (af.GetURL (), af.GetTags ());
	}

	void Aggregator::on_ActionRemoveFeed__triggered ()
	{
		const auto& ds = GetRelevantIndex ();
		if (!ds.isValid ())
			return;

		const auto& name = ds.sibling (ds.row (), ChannelsModel::ColumnTitle).data ().toString ();
		if (QMessageBox::question (nullptr,
					tr ("Feed deletion"),
					tr ("Are you sure you want to delete feed %1?")
						.arg (Util::FormatName (name)),
					QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;

		const auto feedId = ds.data (ChannelRoles::FeedID).value<IDType_t> ();
		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->RemoveFeed (feedId);
	}

	void Aggregator::on_ActionRenameFeed__triggered ()
	{
		const auto& ds = GetRelevantIndex ();
		if (!ds.isValid ())
			return;

		const auto& current = ds.sibling (ds.row (), ChannelsModel::ColumnTitle).data ().toString ();
		const auto& newName = QInputDialog::getText (nullptr,
				tr ("Rename feed"),
				tr ("New feed name:"),
				QLineEdit::Normal,
				current);
		if (newName.isEmpty ())
			return;

		auto sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();
		sb->SetChannelDisplayTitle (ds.data (ChannelRoles::ChannelID).value<IDType_t> (), newName);
	}

	void Aggregator::on_ActionRemoveChannel__triggered ()
	{
		const auto& ds = GetRelevantIndex ();
		if (!ds.isValid ())
			return;

		const auto& name = ds.sibling (ds.row (), ChannelsModel::ColumnTitle).data ().toString ();
		if (QMessageBox::question (nullptr,
				tr ("Channel deletion"),
				tr ("Are you sure you want to delete channel %1?")
					.arg (Util::FormatName (name)),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;

		const auto channelId = ds.data (ChannelRoles::ChannelID).value<IDType_t> ();
		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->RemoveChannel (channelId);
	}

	template<typename F>
	void Aggregator::Perform (F&& func)
	{
		for (auto index : GetRelevantIndexes ())
			func (index);
	}

	namespace
	{
		QString FormatNamesList (const QStringList& names)
		{
			return "<em>" + names.join ("</em>; <em>") + "</em>";
		}
	}

	void Aggregator::on_ActionMarkChannelAsRead__triggered ()
	{
		QStringList names;
		Perform ([&names] (const QModelIndex& mi)
				{ names << mi.sibling (mi.row (), 0).data ().toString (); });
		if (XmlSettingsManager::Instance ()->Property ("ConfirmMarkChannelAsRead", true).toBool ())
		{
			QMessageBox mbox (QMessageBox::Question,
					"LeechCraft",
					tr ("Are you sure you want to mark all items in %1 as read?")
						.arg (FormatNamesList (names)),
					QMessageBox::Yes | QMessageBox::No);

			mbox.setDefaultButton (QMessageBox::Yes);

			QPushButton always (tr ("Always"));
			mbox.addButton (&always, QMessageBox::AcceptRole);

			if (mbox.exec () == QMessageBox::No)
				return;
			else if (mbox.clickedButton () == &always)
				XmlSettingsManager::Instance ()->setProperty ("ConfirmMarkChannelAsRead", false);
		}

		Perform ([] (const QModelIndex& mi) { MarkChannel (mi, false); });
	}

	void Aggregator::on_ActionMarkChannelAsUnread__triggered ()
	{
		QStringList names;
		Perform ([&names] (const QModelIndex& mi)
				{ names << mi.sibling (mi.row (), 0).data ().toString (); });
		if (QMessageBox::question (nullptr,
				"LeechCraft",
				tr ("Are you sure you want to mark all items in %1 as unread?")
					.arg (FormatNamesList (names)),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		Perform ([] (const QModelIndex& mi) { MarkChannel (mi, true); });
	}

	void Aggregator::on_ActionChannelSettings__triggered ()
	{
		QModelIndex index = GetRelevantIndex ();
		if (!index.isValid ())
			return;

		FeedSettings dia { index, Proxy_ };
		dia.exec ();
	}

	void Aggregator::on_ActionUpdateSelectedFeed__triggered ()
	{
		Perform ([] (const QModelIndex& mi)
				{
					const auto feedId = mi.data (ChannelRoles::FeedID).value<IDType_t> ();
					Core::Instance ().UpdateFeed (feedId);
				});
	}

	void Aggregator::on_ActionImportOPML__triggered ()
	{
		Core::Instance ().StartAddingOPML (QString ());
	}

	void Aggregator::on_ActionExportOPML__triggered ()
	{
		ExportUtils::RunExportOPML (Proxy_->GetTagsManager ());
	}

	void Aggregator::on_ActionImportBinary__triggered ()
	{
		ImportBinary import (nullptr);
		if (import.exec () == QDialog::Rejected)
			return;

		Core::Instance ().AddFeeds (import.GetSelectedFeeds (),
				import.GetTags ());
	}

	void Aggregator::on_ActionExportBinary__triggered ()
	{
		ExportUtils::RunExportBinary ();
	}

	void Aggregator::on_ActionExportFB2__triggered ()
	{
		const auto dialog = new Export2FB2Dialog (ChannelsModel_.get (), nullptr);
		dialog->setAttribute (Qt::WA_DeleteOnClose);
		dialog->show ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_aggregator, LeechCraft::Aggregator::Aggregator);
