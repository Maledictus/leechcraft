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

#include "torrenttabwidget.h"
#include <chrono>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <libtorrent/session.hpp>
#include <libtorrent/lazy_entry.hpp>
#include <util/util.h>
#include <util/compat/fontwidth.h>
#include <util/xpc/util.h>
#include <util/tags/tagscompleter.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "peerstablinker.h"
#include "piecesmodel.h"
#include "peersmodel.h"
#include "addpeerdialog.h"
#include "addwebseeddialog.h"
#include "banpeersdialog.h"
#include "sessionsettingsmanager.h"
#include "sessionstats.h"

namespace LC
{
namespace BitTorrent
{
	TorrentTabWidget::TorrentTabWidget (QWidget *parent)
	: QTabWidget (parent)
	, PeersSorter_ (new QSortFilterProxyModel (this))
	{
		Ui_.setupUi (this);
		new Util::TagsCompleter (Ui_.TorrentTags_);
		QHeaderView *header = Ui_.PerTrackerStats_->header ();
		const auto width = [this] (const auto& str) { return Util::Compat::Width (fontMetrics (), str); };
		header->resizeSection (0, width ("www.domain.name.org"));
		header->resizeSection (1, width ("1234.5678 bytes/s"));
		header->resizeSection (2, width ("1234.5678 bytes/s"));

		Ui_.TorrentTags_->AddSelector ();

		PeersSorter_->setDynamicSortFilter (true);
		PeersSorter_->setSortRole (PeersModel::SortRole);
		Ui_.PeersView_->setModel (PeersSorter_);
		connect (Ui_.PeersView_->selectionModel (),
				SIGNAL (currentChanged (const QModelIndex&, const QModelIndex&)),
				this,
				SLOT (currentPeerChanged (const QModelIndex&)));
		new PeersTabLinker (&Ui_, PeersSorter_, this);

		header = Ui_.WebSeedsView_->header ();
		header->resizeSection (0, width ("average.domain.name.of.a.tracker"));
		header->resizeSection (1, width ("  BEP 99  "));

		connect (Ui_.OverallDownloadRateController_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_OverallDownloadRateController__valueChanged (int)));
		connect (Ui_.OverallUploadRateController_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_OverallUploadRateController__valueChanged (int)));
		connect (Ui_.TorrentDownloadRateController_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_TorrentDownloadRateController__valueChanged (int)));
		connect (Ui_.TorrentUploadRateController_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_TorrentUploadRateController__valueChanged (int)));
		connect (Ui_.TorrentManaged_,
				SIGNAL (stateChanged (int)),
				this,
				SLOT (on_TorrentManaged__stateChanged (int)));
		connect (Ui_.TorrentSequentialDownload_,
				SIGNAL (stateChanged (int)),
				this,
				SLOT (on_TorrentSequentialDownload__stateChanged (int)));
		connect (Ui_.TorrentSuperSeeding_,
				SIGNAL (stateChanged (int)),
				this,
				SLOT (on_TorrentSuperSeeding__stateChanged (int)));
		connect (Ui_.DownloadingTorrents_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_DownloadingTorrents__valueChanged (int)));
		connect (Ui_.UploadingTorrents_,
				SIGNAL (valueChanged (int)),
				this,
				SLOT (on_UploadingTorrents__valueChanged (int)));
		connect (Ui_.TorrentTags_,
				SIGNAL (editingFinished ()),
				this,
				SLOT (on_TorrentTags__editingFinished ()));

		connect (Core::Instance (),
				SIGNAL (torrentsStatusesUpdated ()),
				this,
				SLOT (updateTorrentStats ()),
				Qt::QueuedConnection);
		connect (this,
				SIGNAL (currentChanged (int)),
				this,
				SLOT (updateTorrentStats ()));

		UpdateDashboard ();

		AddPeer_ = new QAction (tr ("Add peer..."),
				Ui_.PeersView_);
		AddPeer_->setProperty ("ActionIcon", "list-add-user");
		AddPeer_->setObjectName ("AddPeer_");
		connect (AddPeer_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleAddPeer ()));
		Ui_.PeersView_->addAction (AddPeer_);

		BanPeer_ = new QAction (tr ("Ban peer..."),
				Ui_.PeersView_);
		BanPeer_->setProperty ("ActionIcon", "im-ban-user");
		BanPeer_->setObjectName ("BanPeer_");
		BanPeer_->setEnabled (false);
		connect (BanPeer_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleBanPeer ()));
		Ui_.PeersView_->addAction (BanPeer_);

		AddWebSeed_ = new QAction (tr ("Add web seed..."),
				Ui_.WebSeedsView_);
		AddWebSeed_->setObjectName ("AddWebSeed_");
		connect (AddWebSeed_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleAddWebSeed ()));

		RemoveWebSeed_ = new QAction (tr ("Remove web seed"),
				Ui_.WebSeedsView_);
		RemoveWebSeed_->setProperty ("ActionIcon", "list-remove-user");
		RemoveWebSeed_->setObjectName ("RemoveWebSeed_");
		RemoveWebSeed_->setEnabled (false);
		connect (RemoveWebSeed_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleRemoveWebSeed ()));
		Ui_.WebSeedsView_->addAction (AddWebSeed_);
		Ui_.WebSeedsView_->addAction (RemoveWebSeed_);

		const QList<QByteArray> tabWidgetSettings
		{
			"ActiveSessionStats",
			"ActiveAdvancedSessionStats",
			"ActiveTrackerStats",
			"ActiveCacheStats",
			"ActiveTorrentStatus",
			"ActiveTorrentAdvancedStatus",
			"ActiveTorrentInfo",
			"ActiveTorrentPeers"
		};
		XmlSettingsManager::Instance ()->RegisterObject (tabWidgetSettings, this, "setTorrentTabWidgetSettings");

		setTabWidgetSettings ();
	}

	void TorrentTabWidget::SetChangeTrackersAction (QAction *changeTrackers)
	{
		Ui_.TrackersButton_->setDefaultAction (changeTrackers);
	}

	void TorrentTabWidget::SetCurrentIndex (int index)
	{
		if (Index_ == index)
			return;

		Index_ = index;
		InvalidateSelection ();

		Ui_.FilesWidget_->SetCurrentIndex (Index_);

		const QList<QAbstractItemModel*> oldModels
		{
			Ui_.PiecesView_->model (),
			PeersSorter_->sourceModel (),
			Ui_.WebSeedsView_->model ()
		};

		auto piecesModel = Core::Instance ()->GetPiecesModel (Index_);
		Ui_.PiecesView_->setModel (piecesModel);

		PeersSorter_->setSourceModel (Core::Instance ()->GetPeersModel (Index_));

		Ui_.WebSeedsView_->setModel (Core::Instance ()->GetWebSeedsModel (Index_));
		connect (Ui_.WebSeedsView_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (currentWebSeedChanged (QModelIndex)));

		qDeleteAll (oldModels);
	}

	void TorrentTabWidget::SetSelectedIndices (const QList<int>& indices)
	{
		SelectedIndices_ = indices;
	}

	void TorrentTabWidget::InvalidateSelection ()
	{
		Ui_.TorrentTags_->setText (Core::Instance ()->GetProxy ()->GetTagsManager ()->
				Join (Core::Instance ()->GetTagsForIndex (Index_)));
		updateTorrentStats ();
	}

	void TorrentTabWidget::updateTorrentStats ()
	{
		UpdateDashboard ();
		UpdateOverallStats ();
		UpdateTorrentControl ();
	}

	template<typename F>
	void TorrentTabWidget::ForEachSelected (F&& f) const
	{
		for (const auto& index : SelectedIndices_)
			f (index);
	}

	void TorrentTabWidget::UpdateOverallStats ()
	{
		const auto& stats = Core::Instance ()->GetSessionStats ();

		Ui_.LabelTotalDownloadRate_->setText (Util::MakePrettySize (stats.Rate_.Down_) + tr ("/s"));
		Ui_.LabelTotalUploadRate_->setText (Util::MakePrettySize (stats.Rate_.Up_) + tr ("/s"));

		auto rel = [] (auto fmt, auto num, auto denom, QLabel *down, QLabel *up)
		{
			down->setText (fmt (num.Down_, denom.Down_));
			up->setText (fmt (num.Up_, denom.Up_));
		};

		auto percent = [] (auto t1, auto t2)
		{
			if (t2)
				return " (" + QString::number (t1 * 100.0 / t2, 'f', 1) + "%)";
			else
				return QString {};
		};

		auto speed = [percent] (auto t1, auto t2)
		{
			return Util::MakePrettySize (t1) + QObject::tr ("/s") + percent (t1, t2);
		};
		rel (speed, stats.IPOverheadRate_, stats.Rate_, Ui_.LabelOverheadDownloadRate_, Ui_.LabelOverheadUploadRate_);
		rel (speed, stats.DHTRate_, stats.Rate_, Ui_.LabelDHTDownloadRate_, Ui_.LabelDHTUploadRate_);
		rel (speed, stats.TrackerRate_, stats.Rate_, Ui_.LabelTrackerDownloadRate_, Ui_.LabelTrackerUploadRate_);

		Ui_.LabelTotalDownloaded_->setText (Util::MakePrettySize (stats.Total_.Down_));
		Ui_.LabelTotalUploaded_->setText (Util::MakePrettySize (stats.Total_.Up_));

		auto simple = [percent] (auto t1, auto t2)
		{
			return Util::MakePrettySize (t1) + percent (t1, t2);
		};
		rel (simple, stats.IPOverheadTotal_, stats.Total_, Ui_.LabelOverheadDownloaded_, Ui_.LabelOverheadUploaded_);
		rel (simple, stats.DHTTotal_, stats.Total_, Ui_.LabelDHTDownloaded_, Ui_.LabelDHTUploaded_);
		rel (simple, stats.TrackerTotal_, stats.Total_, Ui_.LabelTrackerDownloaded_, Ui_.LabelTrackerUploaded_);

		Ui_.LabelTotalPeers_->setText (QString::number (stats.NumPeers_));
		Ui_.LabelTotalDHTNodes_->setText ("(" +
				QString::number (stats.DHTGlobalNodes_) +
				") " +
				QString::number (stats.DHTNodes_));
		Ui_.LabelDHTTorrents_->setText (QString::number (stats.DHTTorrents_));
		Ui_.LabelListenPort_->setText (QString::number (Core::Instance ()->GetListenPort ()));
		if (stats.PayloadTotal_.Down_)
			Ui_.LabelSessionRating_->setText (QString::number (stats.PayloadTotal_.Up_ /
					static_cast<double> (stats.PayloadTotal_.Down_), 'g', 4));
		else
			Ui_.LabelSessionRating_->setText (QString::fromUtf8 ("\u221E"));
		Ui_.LabelTotalFailedData_->setText (Util::MakePrettySize (stats.TotalFailedBytes_));
		Ui_.LabelTotalRedundantData_->setText (Util::MakePrettySize (stats.TotalRedundantBytes_));
		Ui_.LabelExternalAddress_->setText (Core::Instance ()->GetExternalAddress ());

		Ui_.BlocksWritten_->setText (QString::number (stats.BlocksWritten_));
		Ui_.Writes_->setText (QString::number (stats.Writes_));
		Ui_.WriteHitRatio_->setText (QString::number (static_cast<double> (stats.BlocksWritten_ - stats.Writes_) / stats.BlocksWritten_));
		Ui_.CacheSize_->setText (QString::number (stats.CacheSize_));
		Ui_.TotalBlocksRead_->setText (QString::number (stats.BlocksRead_));
		Ui_.CachedBlockReads_->setText (QString::number (stats.BlocksReadHit_));
		Ui_.ReadHitRatio_->setText (QString::number (static_cast<double> (stats.BlocksReadHit_) / stats.BlocksRead_));
		Ui_.ReadCacheSize_->setText (QString::number (stats.ReadCacheSize_));

		Core::pertrackerstats_t ptstats;
		Core::Instance ()->GetPerTracker (ptstats);
		Ui_.PerTrackerStats_->clear ();

		for (auto i = ptstats.begin (), end = ptstats.end (); i != end; ++i)
		{
			QStringList strings;
			strings	<< i.key ()
				<< Util::MakePrettySize (i->DownloadRate_) + tr ("/s")
				<< Util::MakePrettySize (i->UploadRate_) + tr ("/s");

			new QTreeWidgetItem (Ui_.PerTrackerStats_, strings);
		}
	}

	void TorrentTabWidget::UpdateDashboard ()
	{
		const auto ssm = Core::Instance ()->GetSessionSettingsManager ();
		Ui_.OverallDownloadRateController_->setValue (ssm->GetOverallDownloadRate ());
		Ui_.OverallUploadRateController_->setValue (ssm->GetOverallUploadRate ());
		Ui_.DownloadingTorrents_->setValue (ssm->GetMaxDownloadingTorrents ());
		Ui_.UploadingTorrents_->setValue (ssm->GetMaxUploadingTorrents ());
	}

	void TorrentTabWidget::UpdateTorrentControl ()
	{
		Ui_.TorrentDownloadRateController_->setValue (Core::Instance ()->GetTorrentDownloadRate (Index_));
		Ui_.TorrentUploadRateController_->setValue (Core::Instance ()->GetTorrentUploadRate (Index_));
		Ui_.TorrentManaged_->setCheckState (Core::Instance ()->
				IsTorrentManaged (Index_) ? Qt::Checked : Qt::Unchecked);
		Ui_.TorrentSequentialDownload_->setCheckState (Core::Instance ()->
				IsTorrentSequentialDownload (Index_) ? Qt::Checked : Qt::Unchecked);
		Ui_.TorrentSuperSeeding_->setCheckState (Core::Instance ()->
				IsTorrentSuperSeeding (Index_) ? Qt::Checked : Qt::Unchecked);

		std::unique_ptr<TorrentInfo> i;
		try
		{
			i = Core::Instance ()->GetTorrentStats (Index_);
		}
		catch (...)
		{
			Ui_.TorrentControlTab_->setEnabled (false);
			return;
		}

		if (!i->Info_)
			i->Info_.reset (new libtorrent::torrent_info { libtorrent::bdecode_node {} });

		Ui_.TorrentControlTab_->setEnabled (true);
		Ui_.LabelState_->setText (i->State_);
		Ui_.LabelDownloadRate_->setText (Util::MakePrettySize (i->Status_.download_rate) + tr ("/s"));
		Ui_.LabelUploadRate_->setText (Util::MakePrettySize (i->Status_.upload_rate) + tr ("/s"));

		const auto nextAnnounceSecs = libtorrent::duration_cast<libtorrent::seconds>(i->Status_.next_announce).count ();
		Ui_.LabelNextAnnounce_->setText (Util::MakeTimeFromLong (nextAnnounceSecs));

		Ui_.LabelProgress_->setText (QString::number (i->Status_.progress * 100, 'f', 2) + "%");
		Ui_.LabelDownloaded_->setText (Util::MakePrettySize (i->Status_.total_download));
		Ui_.LabelUploaded_->setText (Util::MakePrettySize (i->Status_.total_upload));
		Ui_.LabelWantedDownloaded_->setText (Util::MakePrettySize (i->Status_.total_wanted_done));
		Ui_.LabelDownloadedTotal_->setText (Util::MakePrettySize (i->Status_.all_time_download));
		Ui_.LabelUploadedTotal_->setText (Util::MakePrettySize (i->Status_.all_time_upload));
		if (i->Status_.all_time_download)
			Ui_.LabelTorrentOverallRating_->setText (QString::number (i->Status_.all_time_upload /
							static_cast<double> (i->Status_.all_time_download), 'g', 4));
		else
			Ui_.LabelTorrentOverallRating_->setText (QString::fromUtf8 ("\u221E"));
		Ui_.LabelSeedRank_->setText (QString::number (i->Status_.seed_rank));
#if LIBTORRENT_VERSION_NUM >= 10200
		Ui_.LabelActiveTime_->setText (Util::MakeTimeFromLong (i->Status_.active_duration.count ()));
		Ui_.LabelSeedingTime_->setText (Util::MakeTimeFromLong (i->Status_.seeding_duration.count ()));
#else
		Ui_.LabelActiveTime_->setText (Util::MakeTimeFromLong (i->Status_.active_time));
		Ui_.LabelSeedingTime_->setText (Util::MakeTimeFromLong (i->Status_.seeding_time));
#endif
		Ui_.LabelTotalSize_->setText (Util::MakePrettySize (i->Info_->total_size ()));
		Ui_.LabelWantedSize_->setText (Util::MakePrettySize (i->Status_.total_wanted));
		if (i->Status_.total_payload_download)
			Ui_.LabelTorrentRating_->setText (QString::number (i->Status_.total_payload_upload /
							static_cast<double> (i->Status_.total_payload_download), 'g', 4));
		else
			Ui_.LabelTorrentRating_->setText (QString::fromUtf8 ("\u221E"));
		Ui_.PiecesWidget_->setPieceMap (i->Status_.pieces);
		Ui_.LabelTracker_->setText (QString::fromStdString (i->Status_.current_tracker));
		Ui_.LabelDestination_->setText (QString ("<a href='%1'>%1</a>")
					.arg (i->Destination_));
		Ui_.LabelName_->setText (QString::fromStdString (i->Status_.name));
		Ui_.LabelCreator_->setText (QString::fromStdString (i->Info_->creator ()));

		const auto& commentString = QString::fromStdString (i->Info_->comment ());
		if (QUrl::fromEncoded (commentString.toUtf8 ()).isValid ())
			Ui_.LabelComment_->setText (QString ("<a href='%1'>%1</a>")
					.arg (commentString));
		else
			Ui_.LabelComment_->setText (commentString);

		Ui_.LabelPrivate_->setText (i->Info_->priv () ?
					tr ("Yes") :
					tr ("No"));
		Ui_.LabelDHTNodesCount_->setText (QString::number (i->Info_->nodes ().size ()));
		Ui_.LabelFailed_->setText (Util::MakePrettySize (i->Status_.total_failed_bytes));
		Ui_.LabelConnectedPeers_->setText (QString::number (i->Status_.num_peers));
		Ui_.LabelConnectedSeeds_->setText (QString::number (i->Status_.num_seeds));
		Ui_.LabelTotalPieces_->setText (QString::number (i->Info_->num_pieces ()));
		Ui_.LabelDownloadedPieces_->setText (QString::number (i->Status_.num_pieces));
		Ui_.LabelPieceSize_->setText (Util::MakePrettySize (i->Info_->piece_length ()));
		Ui_.LabelBlockSize_->setText (Util::MakePrettySize (i->Status_.block_size));
		Ui_.LabelDistributedCopies_->setText (i->Status_.distributed_copies == -1 ?
					tr ("Not tracking") :
					QString::number (i->Status_.distributed_copies));
		Ui_.LabelRedundantData_->setText (Util::MakePrettySize (i->Status_.total_redundant_bytes));
		Ui_.LabelPeersInList_->setText (QString::number (i->Status_.list_peers));
		Ui_.LabelSeedsInList_->setText (QString::number (i->Status_.list_seeds));
		Ui_.LabelPeersInSwarm_->setText ((i->Status_.num_incomplete == -1 ?
					tr ("Unknown") :
					QString::number (i->Status_.num_incomplete)));
		Ui_.LabelSeedsInSwarm_->setText ((i->Status_.num_complete == -1 ?
					tr ("Unknown") :
					QString::number (i->Status_.num_complete)));
		Ui_.LabelConnectCandidates_->setText (QString::number (i->Status_.connect_candidates));
		Ui_.LabelUpBandwidthQueue_->setText (QString::number (i->Status_.up_bandwidth_queue));
		Ui_.LabelDownBandwidthQueue_->setText (QString::number (i->Status_.down_bandwidth_queue));
	}

	void TorrentTabWidget::on_OverallDownloadRateController__valueChanged (int val)
	{
		Core::Instance ()->GetSessionSettingsManager ()->SetOverallDownloadRate (val);
	}

	void TorrentTabWidget::on_OverallUploadRateController__valueChanged (int val)
	{
		Core::Instance ()->GetSessionSettingsManager ()->SetOverallUploadRate (val);
	}

	void TorrentTabWidget::on_TorrentDownloadRateController__valueChanged (int val)
	{
		ForEachSelected ([val] (int idx) { Core::Instance ()->SetTorrentDownloadRate (val, idx); });
	}

	void TorrentTabWidget::on_TorrentUploadRateController__valueChanged (int val)
	{
		ForEachSelected ([val] (int idx) { Core::Instance ()->SetTorrentUploadRate (val, idx); });
	}

	void TorrentTabWidget::on_TorrentManaged__stateChanged (int state)
	{
		const auto enable = state == Qt::Checked;
		ForEachSelected ([enable] (int idx) { Core::Instance ()->SetTorrentManaged (enable, idx); });
	}

	void TorrentTabWidget::on_TorrentSequentialDownload__stateChanged (int state)
	{
		const auto enable = state == Qt::Checked;
		ForEachSelected ([enable] (int idx) { Core::Instance ()->SetTorrentSequentialDownload (enable, idx); });
	}

	void TorrentTabWidget::on_TorrentSuperSeeding__stateChanged (int state)
	{
		const auto enable = state == Qt::Checked;
		ForEachSelected ([enable] (int idx) { Core::Instance ()->SetTorrentSuperSeeding (enable, idx); });
	}

	void TorrentTabWidget::on_DownloadingTorrents__valueChanged (int newValue)
	{
		Core::Instance ()->GetSessionSettingsManager ()->SetMaxDownloadingTorrents (newValue);
	}

	void TorrentTabWidget::on_UploadingTorrents__valueChanged (int newValue)
	{
		Core::Instance ()->GetSessionSettingsManager ()->SetMaxUploadingTorrents (newValue);
	}

	void TorrentTabWidget::on_TorrentTags__editingFinished ()
	{
		const auto& tags = Core::Instance ()->GetProxy ()->GetTagsManager ()->Split (Ui_.TorrentTags_->text ());
		ForEachSelected ([&tags] (int idx) { Core::Instance ()->UpdateTags (tags, idx); });
	}

	void TorrentTabWidget::on_LabelComment__linkActivated (const QString& link)
	{
		const auto& e = Util::MakeEntity (QUrl::fromEncoded (link.toUtf8 ()),
				{},
				TaskParameter::FromUserInitiated | TaskParameter::OnlyHandle);
		Core::Instance ()->GetProxy ()->GetEntityManager ()->HandleEntity (e);
	}

	void TorrentTabWidget::setTabWidgetSettings ()
	{
		Ui_.BoxSessionStats_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveSessionStats").toBool ());
		Ui_.BoxAdvancedSessionStats_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveAdvancedSessionStats").toBool ());
		Ui_.BoxPerTrackerStats_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveTrackerStats").toBool ());
		Ui_.BoxCacheStats_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveCacheStats").toBool ());
		Ui_.BoxTorrentStatus_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveTorrentStatus").toBool ());
		Ui_.BoxTorrentAdvancedStatus_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveTorrentAdvancedStatus").toBool ());
		Ui_.BoxTorrentInfo_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveTorrentInfo").toBool ());
		Ui_.BoxTorrentPeers_->setVisible (XmlSettingsManager::Instance ()->
				property ("ActiveTorrentPeers").toBool ());
	}

	void TorrentTabWidget::handleAddPeer ()
	{
		AddPeerDialog peer;
		if (peer.exec () != QDialog::Accepted)
			return;

		Core::Instance ()->AddPeer (peer.GetIP (), peer.GetPort (), Index_);
	}

	void TorrentTabWidget::handleBanPeer ()
	{
		QModelIndex peerIndex = Ui_.PeersView_->currentIndex ();

		BanPeersDialog ban;
		ban.SetIP (peerIndex.sibling (peerIndex.row (), 0).data ().toString ());
		if (ban.exec () != QDialog::Accepted)
			return;

		Core::Instance ()->BanPeers (qMakePair (ban.GetStart (), ban.GetEnd ()));
	}

	void TorrentTabWidget::handleAddWebSeed ()
	{
		AddWebSeedDialog ws;
		if (ws.exec () != QDialog::Accepted ||
				ws.GetURL ().isEmpty ())
			return;

		if (!QUrl (ws.GetURL ()).isValid ())
			return;

		Core::Instance ()->AddWebSeed (ws.GetURL (), ws.GetType (), Index_);
	}

	void TorrentTabWidget::currentPeerChanged (const QModelIndex& index)
	{
		BanPeer_->setEnabled (index.isValid ());
	}

	void TorrentTabWidget::currentWebSeedChanged (const QModelIndex& index)
	{
		RemoveWebSeed_->setEnabled (index.isValid ());
	}

	void TorrentTabWidget::handleRemoveWebSeed ()
	{
		QModelIndex index = Ui_.WebSeedsView_->currentIndex ();
		QString url = index.sibling (index.row (), 0).data ().toString ();
		bool bep19 = index.sibling (index.row (), 1).data ().toString () == "BEP 19";
		Core::Instance ()->RemoveWebSeed (index.data ().toString (), bep19, Index_);
	}
}
}
