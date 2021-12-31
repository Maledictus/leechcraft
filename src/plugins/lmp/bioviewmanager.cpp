/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "bioviewmanager.h"
#include <algorithm>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QtConcurrentRun>
#include <QStandardItemModel>
#include <util/util.h>
#include <util/qml/colorthemeproxy.h>
#include <util/qml/themeimageprovider.h>
#include <util/sys/paths.h>
#include <util/models/roleditemsmodel.h>
#include <util/sll/prelude.h>
#include <util/sll/visitor.h>
#include <util/threads/futures.h>
#include <interfaces/media/idiscographyprovider.h>
#include <interfaces/media/ialbumartprovider.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include "biopropproxy.h"
#include "core.h"
#include "util.h"
#include "previewhandler.h"
#include "stdartistactionsmanager.h"
#include "localcollection.h"

namespace LC::LMP
{
	namespace
	{
		constexpr int AASize = 170;
	}

	struct BioViewManager::DiscoItem
	{
		QString Name_;
		QString Year_;
		QUrl Image_;
		QString TrackListToolTip_;
	};

	BioViewManager::BioViewManager (QQuickWidget *view, QObject *parent)
	: QObject { parent }
	, View_ { view }
	, BioPropProxy_ { new BioPropProxy { this } }
	, DiscoModel_ { new DiscoModel {
		this,
		Util::RoledMemberField_v<"albumName", &DiscoItem::Name_>,
		Util::RoledMemberField_v<"albumYear", &DiscoItem::Year_>,
		Util::RoledMemberField_v<"albumImage", &DiscoItem::Image_>,
		Util::RoledMemberField_v<"albumTrackListTooltip", &DiscoItem::TrackListToolTip_>,
	} }
	{
		View_->rootContext ()->setContextObject (BioPropProxy_);
		View_->rootContext ()->setContextProperty ("artistDiscoModel", DiscoModel_);
		View_->rootContext ()->setContextProperty ("colorProxy",
				new Util::ColorThemeProxy (GetProxyHolder ()->GetColorThemeManager (), this));
		View_->engine ()->addImageProvider ("ThemeIcons", new Util::ThemeImageProvider (GetProxyHolder ()));

		for (const auto& cand : Util::GetPathCandidates (Util::SysPath::QML, ""))
			View_->engine ()->addImportPath (cand);
	}

	void BioViewManager::InitWithSource ()
	{
		connect (View_->rootObject (),
				SIGNAL (albumPreviewRequested (int)),
				this,
				SLOT (handleAlbumPreviewRequested (int)));

		new StdArtistActionsManager (View_, this);
	}

	void BioViewManager::Request (Media::IArtistBioFetcher *fetcher, const QString& artist, const QStringList& releases)
	{
		// Only clear the models if the artist is different to avoid flicker in the (most common) case
		// of the artist being the same.
		if (CurrentArtist_ != artist)
		{
			DiscoModel_->SetItems ({});
			BioPropProxy_->SetBio ({});

			CurrentArtist_ = artist;
		}

		Util::Sequence (this, fetcher->RequestArtistBio (CurrentArtist_)) >>
				Util::Visitor
				{
					[this] (const QString&) { BioPropProxy_->SetBio ({}); },
					[this] (const Media::ArtistBio& bio)
					{
						BioPropProxy_->SetBio (bio);
						emit gotArtistImage (bio.BasicInfo_.Name_, bio.BasicInfo_.LargeImage_);
					}
				};

		auto pm = GetProxyHolder ()->GetPluginsManager ();
		for (auto prov : pm->GetAllCastableTo<Media::IDiscographyProvider*> ())
			Util::Sequence (this, prov->GetDiscography (CurrentArtist_, releases)) >>
					Util::Visitor
					{
						[artist] (const QString&) { qWarning () << Q_FUNC_INFO << "error for" << artist; },
						[this] (const auto& releases) { HandleDiscographyReady (releases); }
					};
	}

	std::optional<int> BioViewManager::FindAlbumItem (const QString& albumName) const
	{
		const auto& albums = DiscoModel_->GetItems ();
		const auto pos = std::find_if (albums.begin (), albums.end (),
				[&] (const DiscoItem& album) { return album.Name_ == albumName; });
		if (pos == albums.end ())
			return {};

		return pos - albums.begin ();
	}

	bool BioViewManager::QueryReleaseImageLocal (const Media::AlbumInfo& info) const
	{
		const auto coll = Core::Instance ().GetLocalCollection ();
		const auto albumId = coll->FindAlbum (info.Artist_, info.Album_);
		if (albumId == -1)
			return false;

		const auto& album = coll->GetAlbum (albumId);
		if (!album)
			return false;

		const auto& path = album->CoverPath_;
		if (path.isEmpty () || !QFile::exists (path))
			return false;

		SetAlbumImage (info.Album_, QUrl::fromLocalFile (path));
		return true;
	}

	void BioViewManager::QueryReleaseImage (Media::IAlbumArtProvider *aaProv, const Media::AlbumInfo& info)
	{
		if (QueryReleaseImageLocal (info))
			return;

		Util::Sequence (this, aaProv->RequestAlbumArt (info)) >>
				Util::Visitor
				{
					[this, info] (const QList<QUrl>& urls)
					{
						if (info.Artist_ == CurrentArtist_ && !urls.isEmpty ())
							SetAlbumImage (info.Album_, urls.first ());
					},
					[] (const QString&) {}
				};
	}

	void BioViewManager::SetAlbumImage (const QString& album, const QUrl& img) const
	{
		auto idx = FindAlbumItem (album);
		if (!idx)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown item for"
					<< album;
			return;
		}

		DiscoModel_->EditItem (*idx, [&] (DiscoItem& item) { item.Image_ = img; });
	}

	void BioViewManager::HandleDiscographyReady (QList<Media::ReleaseInfo> releases)
	{
		const auto pm = GetProxyHolder ()->GetPluginsManager ();
		const auto aaProvObj = pm->GetAllCastableRoots<Media::IAlbumArtProvider*> ().value (0);
		const auto aaProv = qobject_cast<Media::IAlbumArtProvider*> (aaProvObj);

		const auto& icon = GetProxyHolder ()->GetIconThemeManager ()->
				GetIcon ("media-optical").pixmap (AASize * 2, AASize * 2);
		const auto& iconBase64 = Util::GetAsBase64Src (icon.toImage ());

		std::sort (releases.rbegin (), releases.rend (),
				Util::ComparingBy (&Media::ReleaseInfo::Year_));

		QVector<DiscoItem> newItems;
		newItems.reserve (releases.size ());
		for (const auto& release : releases)
		{
			if (FindAlbumItem (release.Name_))
				continue;

			newItems.push_back ({
					.Name_ = release.Name_,
					.Year_ = QString::number (release.Year_),
					.Image_ = iconBase64,
					.TrackListToolTip_ = MakeTrackListTooltip (release.TrackInfos_),
				});

			Album2Tracks_ << Util::Concat (release.TrackInfos_);
			QueryReleaseImage (aaProv, { CurrentArtist_, release.Name_ });
		}
		DiscoModel_->SetItems (DiscoModel_->GetItems () + std::move (newItems));
	}

	void BioViewManager::handleAlbumPreviewRequested (int index)
	{
		QList<QPair<QString, int>> tracks;
		for (const auto& track : Album2Tracks_.at (index))
			tracks.push_back ({ track.Name_, track.Length_ });

		auto ph = Core::Instance ().GetPreviewHandler ();
		ph->previewAlbum (CurrentArtist_, DiscoModel_->GetItems () [index].Name_, tracks);
	}
}
