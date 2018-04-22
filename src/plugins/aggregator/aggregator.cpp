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
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "itemsfiltermodel.h"
#include "channelsfiltermodel.h"
#include "aggregator.h"
#include "core.h"
#include "addfeed.h"
#include "itemsfiltermodel.h"
#include "channelsfiltermodel.h"
#include "xmlsettingsmanager.h"
#include "export.h"
#include "importbinary.h"
#include "feedsettings.h"
#include "jobholderrepresentation.h"
#include "wizardgenerator.h"
#include "export2fb2dialog.h"
#include "actionsstructs.h"
#include "itemswidget.h"
#include "channelsmodel.h"
#include "aggregatortab.h"
#include "channelsmodelrepresentationproxy.h"

namespace LeechCraft
{
namespace Aggregator
{
	using LeechCraft::Util::CategorySelector;
	using LeechCraft::ActionInfo;

	struct Aggregator_Impl
	{
		AppWideActions AppWideActions_;
		ChannelActions ChannelActions_;

		QMenu *ToolMenu_;

		std::shared_ptr<Util::XmlSettingsDialog> XmlSettingsDialog_;

		ItemsWidget *ReprWidget_ = nullptr;
		ChannelsModelRepresentationProxy *ReprModel_ = nullptr;
		QModelIndex SelectedRepr_;

		TabClassInfo TabInfo_;
		std::unique_ptr<AggregatorTab> AggregatorTab_;

		bool InitFailed_;
	};

	void Aggregator::Init (ICoreProxy_ptr proxy)
	{
		Impl_ = new Aggregator_Impl;
		Impl_->InitFailed_ = false;
		Util::InstallTranslator ("aggregator");

		Impl_->TabInfo_ = TabClassInfo
		{
			"Aggregator",
			GetName (),
			GetInfo (),
			GetIcon (),
			0,
			TFSingle | TFOpenableByRequest
		};

		Impl_->ChannelActions_.SetupActionsStruct (this);
		Impl_->AppWideActions_.SetupActionsStruct (this);
		Core::Instance ().SetAppWideActions (Impl_->AppWideActions_);

		Impl_->ToolMenu_ = new QMenu (tr ("Aggregator"));
		Impl_->ToolMenu_->setIcon (GetIcon ());
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionMarkAllAsRead_);
		Impl_->ToolMenu_->addSeparator ();
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionImportOPML_);
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionExportOPML_);
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionImportBinary_);
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionExportBinary_);
		Impl_->ToolMenu_->addAction (Impl_->AppWideActions_.ActionExportFB2_);

		Core::Instance ().SetProxy (proxy);

		Impl_->XmlSettingsDialog_ = std::make_shared<Util::XmlSettingsDialog> ();
		Impl_->XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (), "aggregatorsettings.xml");
		Impl_->XmlSettingsDialog_->SetCustomWidget ("BackendSelector",
				new Util::BackendSelector (XmlSettingsManager::Instance ()));

		if (!Core::Instance ().DoDelayedInit ())
		{
			Impl_->AppWideActions_.ActionAddFeed_->setEnabled (false);
			Impl_->AppWideActions_.ActionUpdateFeeds_->setEnabled (false);
			Impl_->AppWideActions_.ActionImportOPML_->setEnabled (false);
			Impl_->AppWideActions_.ActionExportOPML_->setEnabled (false);
			Impl_->AppWideActions_.ActionImportBinary_->setEnabled (false);
			Impl_->AppWideActions_.ActionExportBinary_->setEnabled (false);
			Impl_->AppWideActions_.ActionExportFB2_->setEnabled (false);
			Impl_->InitFailed_ = true;
			qWarning () << Q_FUNC_INFO
				<< "core initialization failed";
		}

		if (Impl_->InitFailed_)
		{
			auto box = new QMessageBox (QMessageBox::Critical,
					"LeechCraft",
					tr ("Aggregator failed to initialize properly. Check logs and talk with "
						"the developers. Or, at least, check the storage backend settings and "
						"restart LeechCraft.<br /><br />If you are using SQLite backend (the "
						"default), make sure you have the corresponding Qt driver installed."),
					QMessageBox::Ok);
			box->open ();
		}

		Impl_->ReprWidget_ = new ItemsWidget;
		Impl_->ReprWidget_->SetChannelsFilter (Core::Instance ().GetJobHolderRepresentation ());
		Impl_->ReprWidget_->RegisterShortcuts ();
		Impl_->ReprWidget_->SetAppWideActions (Impl_->AppWideActions_);
		Impl_->ReprWidget_->SetChannelActions (Impl_->ChannelActions_);

		Impl_->ReprModel_ = new ChannelsModelRepresentationProxy { this };
		Impl_->ReprModel_->setSourceModel (Core::Instance ().GetJobHolderRepresentation ());
		Impl_->ReprModel_->SetWidgets (Impl_->ReprWidget_->GetToolBar (), Impl_->ReprWidget_);

		QMenu *contextMenu = new QMenu (tr ("Feeds actions"));
		contextMenu->addAction (Impl_->ChannelActions_.ActionMarkChannelAsRead_);
		contextMenu->addAction (Impl_->ChannelActions_.ActionMarkChannelAsUnread_);
		contextMenu->addSeparator ();
		contextMenu->addAction (Impl_->ChannelActions_.ActionRemoveFeed_);
		contextMenu->addAction (Impl_->ChannelActions_.ActionUpdateSelectedFeed_);
		contextMenu->addAction (Impl_->ChannelActions_.ActionRenameFeed_);
		contextMenu->addSeparator ();
		contextMenu->addAction (Impl_->ChannelActions_.ActionChannelSettings_);
		contextMenu->addSeparator ();
		contextMenu->addAction (Impl_->AppWideActions_.ActionAddFeed_);
		Impl_->ReprModel_->SetMenu (contextMenu);

		connect (Impl_->AppWideActions_.ActionUpdateFeeds_,
				SIGNAL (triggered ()),
				&Core::Instance (),
				SLOT (updateFeeds ()));

		BuildID2ActionTupleMap ();

		QMetaObject::connectSlotsByName (this);
	}

	void Aggregator::SecondInit ()
	{
		if (Impl_->InitFailed_)
			return;

		Impl_->ReprWidget_->ConstructBrowser ();
	}

	void Aggregator::Release ()
	{
		disconnect (&Core::Instance (), 0, this, 0);
		if (Core::Instance ().GetChannelsModel ())
			disconnect (Core::Instance ().GetChannelsModel (), 0, this, 0);
		delete Impl_;
		Core::Instance ().Release ();
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
		return { Impl_->TabInfo_ };
	}

	void Aggregator::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "Aggregator")
		{
			if (!Impl_->AggregatorTab_)
			{
				Impl_->AggregatorTab_ = std::make_unique<AggregatorTab> (Impl_->AppWideActions_,
						Impl_->ChannelActions_, Impl_->TabInfo_, this);
				connect (Impl_->AggregatorTab_.get (),
						&AggregatorTab::removeTabRequested,
						[this] { emit removeTab (Impl_->AggregatorTab_.get ()); });
			}
			emit addNewTab (Impl_->AggregatorTab_->GetTabClassInfo ().VisibleName_, Impl_->AggregatorTab_.get ());

		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	Util::XmlSettingsDialog_ptr Aggregator::GetSettingsDialog () const
	{
		return Impl_->XmlSettingsDialog_;
	}

	QAbstractItemModel* Aggregator::GetRepresentation () const
	{
		return Impl_->ReprModel_;
	}

	void Aggregator::handleTasksTreeSelectionCurrentRowChanged (const QModelIndex& index, const QModelIndex&)
	{
		QModelIndex si = Core::Instance ().GetProxy ()->MapToSource (index);
		if (si.model () != GetRepresentation ())
			si = QModelIndex ();
		si = Impl_->ReprModel_->mapToSource (si);
		si = Core::Instance ().GetJobHolderRepresentation ()->SelectionChanged (si);
		Impl_->SelectedRepr_ = si;
		Impl_->ReprWidget_->CurrentChannelChanged (si);
	}

	EntityTestHandleResult Aggregator::CouldHandle (const Entity& e) const
	{
		EntityTestHandleResult r;
		if (Core::Instance ().CouldHandle (e))
			r.HandlePriority_ = 1000;
		return r;
	}

	void Aggregator::Handle (Entity e)
	{
		Core::Instance ().Handle (e);
	}

	void Aggregator::SetShortcut (const QString& name, const QKeySequences_t& shortcuts)
	{
		Core::Instance ().GetShortcutManager ()->SetShortcut (name, shortcuts);
	}

	QMap<QString, ActionInfo> Aggregator::GetActionInfo () const
	{
		return Core::Instance ().GetShortcutManager ()->GetActionInfo ();
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
			result << Impl_->ToolMenu_->menuAction ();
			break;
		case ActionsEmbedPlace::CommonContextMenu:
			result << Impl_->AppWideActions_.ActionAddFeed_;
			result << Impl_->AppWideActions_.ActionUpdateFeeds_;
			break;
		case ActionsEmbedPlace::TrayMenu:
			result << Impl_->AppWideActions_.ActionMarkAllAsRead_;
			result << Impl_->AppWideActions_.ActionAddFeed_;
			result << Impl_->AppWideActions_.ActionUpdateFeeds_;
			break;
		default:
			break;
		}

		return result;
	}

	QSet<QByteArray> Aggregator::GetExpectedPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Aggregator.GeneralPlugin/1.0";
		return result;
	}

	void Aggregator::AddPlugin (QObject *plugin)
	{
		Core::Instance ().AddPlugin (plugin);
	}

	void Aggregator::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& recInfo : infos)
		{
			if (recInfo.Data_ == "aggregatortab")
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);

				TabOpenRequested (Impl_->TabInfo_.TabClass_);
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

	bool Aggregator::IsRepr () const
	{
		return Impl_->ReprWidget_->isVisible ();
	}

	QModelIndex Aggregator::GetRelevantIndex () const
	{
		if (IsRepr ())
			return Core::Instance ().GetJobHolderRepresentation ()->mapToSource (Impl_->SelectedRepr_);
		else
			return Impl_->AggregatorTab_->GetRelevantIndex ();
	}

	QList<QModelIndex> Aggregator::GetRelevantIndexes () const
	{
		if (IsRepr ())
			return { Core::Instance ().GetJobHolderRepresentation ()->mapToSource (Impl_->SelectedRepr_) };
		else
			return Impl_->AggregatorTab_->GetRelevantIndexes ();
	}

	void Aggregator::BuildID2ActionTupleMap ()
	{
		typedef Util::ShortcutManager::IDPair_t ID_t;
		auto mgr = Core::Instance ().GetShortcutManager ();
		*mgr << ID_t ("ActionAddFeed", Impl_->AppWideActions_.ActionAddFeed_)
				<< ID_t ("ActionUpdateFeeds_", Impl_->AppWideActions_.ActionUpdateFeeds_)
				<< ID_t ("ActionImportOPML_", Impl_->AppWideActions_.ActionImportOPML_)
				<< ID_t ("ActionExportOPML_", Impl_->AppWideActions_.ActionExportOPML_)
				<< ID_t ("ActionImportBinary_", Impl_->AppWideActions_.ActionImportBinary_)
				<< ID_t ("ActionExportBinary_", Impl_->AppWideActions_.ActionExportBinary_)
				<< ID_t ("ActionExportFB2_", Impl_->AppWideActions_.ActionExportFB2_)
				<< ID_t ("ActionRemoveFeed_", Impl_->ChannelActions_.ActionRemoveFeed_)
				<< ID_t ("ActionUpdateSelectedFeed_", Impl_->ChannelActions_.ActionUpdateSelectedFeed_)
				<< ID_t ("ActionMarkChannelAsRead_", Impl_->ChannelActions_.ActionMarkChannelAsRead_)
				<< ID_t ("ActionMarkChannelAsUnread_", Impl_->ChannelActions_.ActionMarkChannelAsUnread_)
				<< ID_t ("ActionChannelSettings_", Impl_->ChannelActions_.ActionChannelSettings_);
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
				XmlSettingsManager::Instance ()->
						setProperty ("ConfirmMarkAllAsRead", false);
		}

		/* TODO
		QModelIndexList indexes;
		QAbstractItemModel *model = Impl_->Ui_.Feeds_->model ();
		for (int i = 0, size = model->rowCount (); i < size; ++i)
		{
			auto index = model->index (i, 0);
			if (Impl_->FlatToFolders_->GetSourceModel ())
				index = Impl_->FlatToFolders_->MapToSource (index);
			indexes << Core::Instance ().GetChannelsModel ()->mapToSource (index);
		}

		int row = 0;
		for (const auto& index : indexes)
		{
			if (index.isValid ())
				Core::Instance ().MarkChannelAsRead (index);
			else if (Impl_->FlatToFolders_->GetSourceModel ())
			{
				const auto& parentIndex = Impl_->FlatToFolders_->index (row++, 0);
				for (int i = 0, size = model->rowCount (parentIndex); i < size; ++i)
				{
					auto source = Impl_->FlatToFolders_->index (i, 0, parentIndex);
					source = Impl_->FlatToFolders_->MapToSource (source);
					Core::Instance ().MarkChannelAsRead (source);
				}
			}
		}
		 */
	}

	void Aggregator::on_ActionAddFeed__triggered ()
	{
		AddFeed af (QString (), nullptr);
		if (af.exec () == QDialog::Accepted)
			Core::Instance ().AddFeed (af.GetURL (), af.GetTags ());
	}

	void Aggregator::on_ActionRemoveFeed__triggered ()
	{
		QModelIndex ds = GetRelevantIndex ();

		if (!ds.isValid ())
			return;

		QString name = ds.sibling (ds.row (), 0).data ().toString ();

		QMessageBox mb (QMessageBox::Warning,
				"LeechCraft",
				tr ("You are going to permanently remove the feed:"
					"<br />%1<br /><br />"
					"Are you really sure that you want to do it?",
					"Feed removal confirmation").arg (name),
				QMessageBox::Ok | QMessageBox::Cancel,
				nullptr);
		mb.setWindowModality (Qt::WindowModal);
		if (mb.exec () == QMessageBox::Ok)
			Core::Instance ().RemoveFeed (ds);
	}

	void Aggregator::on_ActionRenameFeed__triggered ()
	{
		const auto& ds = GetRelevantIndex ();

		if (!ds.isValid ())
			return;

		const auto& current = ds.sibling (ds.row (), ChannelsModel::ColumnTitle)
				.data ().toString ();
		const QString& newName = QInputDialog::getText (nullptr,
				tr ("Rename feed"),
				tr ("New feed name:"),
				QLineEdit::Normal,
				current);
		if (newName.isEmpty ())
			return;

		Core::Instance ().RenameFeed (ds, newName);
	}

	void Aggregator::on_ActionRemoveChannel__triggered ()
	{
		QModelIndex ds = GetRelevantIndex ();

		if (!ds.isValid ())
			return;

		QString name = ds.sibling (ds.row (), 0).data ().toString ();

		QMessageBox mb (QMessageBox::Warning,
				"LeechCraft",
				tr ("You are going to remove the channel:"
					"<br />%1<br /><br />"
					"Are you really sure that you want to do it?",
					"Channel removal confirmation").arg (name),
				QMessageBox::Ok | QMessageBox::Cancel,
				nullptr);
		mb.setWindowModality (Qt::WindowModal);
		if (mb.exec () == QMessageBox::Ok)
			Core::Instance ().RemoveChannel (ds);
	}

	void Aggregator::Perform (boost::function<void (const QModelIndex&)> func)
	{
		for (auto index : GetRelevantIndexes ())
			func (index);
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
						.arg ("<em>" + names.join ("</em>; <em>") + "</em>"),
					QMessageBox::Yes | QMessageBox::No);

			mbox.setDefaultButton (QMessageBox::Yes);

			QPushButton always (tr ("Always"));
			mbox.addButton (&always, QMessageBox::AcceptRole);

			if (mbox.exec () == QMessageBox::No)
				return;
			else if (mbox.clickedButton () == &always)
				XmlSettingsManager::Instance ()->setProperty ("ConfirmMarkChannelAsRead", false);
		}

		Perform ([] (const QModelIndex& mi) { Core::Instance ().MarkChannelAsRead (mi); });
	}

	void Aggregator::on_ActionMarkChannelAsUnread__triggered ()
	{
		QStringList names;
		Perform ([&names] (const QModelIndex& mi)
				{ names << mi.sibling (mi.row (), 0).data ().toString (); });
		if (QMessageBox::question (nullptr,
				"LeechCraft",
				tr ("Are you sure you want to mark all items in %1 as unread?")
					.arg ("<em>" + names.join ("</em>; <em>") + "</em>"),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		Perform ([] (const QModelIndex& mi) { Core::Instance ().MarkChannelAsUnread (mi); });
	}

	void Aggregator::on_ActionChannelSettings__triggered ()
	{
		QModelIndex index = GetRelevantIndex ();
		if (!index.isValid ())
			return;

		FeedSettings dia { index, nullptr };
		dia.exec ();
	}

	void Aggregator::on_ActionUpdateSelectedFeed__triggered ()
	{
		Perform ([] (const QModelIndex& mi) { Core::Instance ().UpdateFeed (mi); });
	}

	void Aggregator::on_ActionImportOPML__triggered ()
	{
		Core::Instance ().StartAddingOPML (QString ());
	}

	void Aggregator::on_ActionExportOPML__triggered ()
	{
		Export exportDialog (tr ("Export to OPML"),
				tr ("Select save file"),
				tr ("OPML files (*.opml);;"
					"XML files (*.xml);;"
					"All files (*.*)"), nullptr);
		channels_shorts_t channels;
		Core::Instance ().GetChannels (channels);
		exportDialog.SetFeeds (channels);
		if (exportDialog.exec () == QDialog::Rejected)
			return;

		Core::Instance ().ExportToOPML (exportDialog.GetDestination (),
				exportDialog.GetTitle (),
				exportDialog.GetOwner (),
				exportDialog.GetOwnerEmail (),
				exportDialog.GetSelectedFeeds ());
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
		Export exportDialog (tr ("Export to binary file"),
				tr ("Select save file"),
				tr ("Aggregator exchange files (*.lcae);;"
					"All files (*.*)"), nullptr);
		channels_shorts_t channels;
		Core::Instance ().GetChannels (channels);
		exportDialog.SetFeeds (channels);
		if (exportDialog.exec () == QDialog::Rejected)
			return;

		Core::Instance ().ExportToBinary (exportDialog.GetDestination (),
				exportDialog.GetTitle (),
				exportDialog.GetOwner (),
				exportDialog.GetOwnerEmail (),
				exportDialog.GetSelectedFeeds ());
	}

	void Aggregator::on_ActionExportFB2__triggered ()
	{
		Export2FB2Dialog *dialog = new Export2FB2Dialog (nullptr);
		connect (dialog,
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		dialog->setAttribute (Qt::WA_DeleteOnClose);
		dialog->show ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_aggregator, LeechCraft::Aggregator::Aggregator);
