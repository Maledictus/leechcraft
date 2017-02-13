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

#include "loghandler.h"
#include <QFile>
#include <QtDebug>
#include <util/sll/prelude.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/lmp/ilocalcollection.h>
#include <util/sll/slotclosure.h>
#include <util/sll/views.h>
#include <util/sll/qtutil.h>
#include <util/sll/curry.h>
#include <util/sll/functor.h>
#include <util/sll/monad.h>
#include <util/sll/monadplus.h>
#include "parser.h"
#include "tracksselectordialog.h"

namespace LeechCraft
{
namespace LMP
{
namespace PPL
{
	namespace
	{
		class LocalCollectionScrobbler : public QObject
									   , public Media::IAudioScrobbler
		{
			ILocalCollection * const Coll_;
		public:
			LocalCollectionScrobbler (ILocalCollection*, QObject*);

			bool SupportsFeature (Feature feature) const override;
			QString GetServiceName () const override;
			void NowPlaying (const Media::AudioInfo& audio) override;
			void SendBackdated (const BackdatedTracks_t& list) override;
			void PlaybackStopped () override;
			void LoveCurrentTrack () override;
			void BanCurrentTrack () override;
		};

		LocalCollectionScrobbler::LocalCollectionScrobbler (ILocalCollection* coll, QObject *parent)
		: QObject { parent }
		, Coll_ { coll }
		{
		}

		bool LocalCollectionScrobbler::SupportsFeature (Media::IAudioScrobbler::Feature) const
		{
			return false;
		}

		QString LocalCollectionScrobbler::GetServiceName () const
		{
			return tr ("Local collection");
		}

		void LocalCollectionScrobbler::NowPlaying (const Media::AudioInfo&)
		{
		}

		template<typename T, typename F>
		boost::optional<T> FindAttrRelaxed (const QList<T>& items, const QString& attr, F&& attrGetter)
		{
			auto finder = [&] (const auto& checker) -> boost::optional<T>
			{
				const auto pos = std::find_if (items.begin (), items.end (),
						[&] (const auto& item) { return checker (Util::Invoke (attrGetter, item)); });
				if (pos == items.end ())
					return {};
				return *pos;
			};

			auto normalize = [] (const QString& str)
			{
				return str.toLower ().remove (' ');
			};
			const auto& attrNorm = normalize (attr);

			return Util::Msum ({
						finder (Util::Curry (std::equal_to<> {}) (attr)),
						finder ([&] (const QString& other) { return attr.compare (other, Qt::CaseInsensitive); }),
						finder ([&] (const QString& other) { return normalize (other) == attrNorm; })
					});
		}

		boost::optional<Collection::Artist> FindArtist (const Collection::Artists_t& artists, const QString& name)
		{
			return FindAttrRelaxed (artists, name, &Collection::Artist::Name_);
		}

		boost::optional<Collection::Album_ptr> FindAlbum (const QList<Collection::Album_ptr>& albums, const QString& name)
		{
			return FindAttrRelaxed (albums, name, [] (const auto& albumPtr) { return albumPtr->Name_; });
		}

		boost::optional<Collection::Track> FindTrack (const QList<Collection::Track>& tracks, const QString& name)
		{
			return FindAttrRelaxed (tracks, name, &Collection::Track::Name_);
		}

		boost::optional<int> FindMetadata (const Collection::Artists_t& artists, const Media::AudioInfo& info)
		{
			using Util::operator>>;
			using Util::operator*;

			const auto& maybeTrack = FindArtist (artists, info.Artist_) >>
					[&] (const auto& artist) { return FindAlbum (artist.Albums_, info.Album_); } >>
					[&] (const auto& album) { return FindTrack (album->Tracks_, info.Title_); };
			return &Collection::Track::ID_ * maybeTrack;
		}

		void LocalCollectionScrobbler::SendBackdated (const Media::IAudioScrobbler::BackdatedTracks_t& list)
		{
			const auto& artists = Coll_->GetAllArtists ();

			for (const auto& item : list)
				if (const auto& maybeTrackId = FindMetadata (artists, item.first))
					Coll_->RecordPlayedTrack (*maybeTrackId, item.second);
		}

		void LocalCollectionScrobbler::PlaybackStopped ()
		{
		}

		void LocalCollectionScrobbler::LoveCurrentTrack ()
		{
		}

		void LocalCollectionScrobbler::BanCurrentTrack ()
		{
		}
	}

	LogHandler::LogHandler (const QString& logPath,
			ILocalCollection *coll, IPluginsManager *ipm, QObject *parent)
	: QObject { parent }
	, Collection_ { coll }
	{
		QFile file { logPath };
		if (!file.open (QIODevice::ReadOnly))
			return;

		const auto& tracks = ParseData (file.readAll ());
		if (tracks.isEmpty ())
		{
			deleteLater ();
			return;
		}

		auto local = new LocalCollectionScrobbler { coll, this };

		const auto& scrobblers = QList<Media::IAudioScrobbler*> { local } +
				Util::Filter (ipm->GetAllCastableTo<Media::IAudioScrobbler*> (),
						[] (Media::IAudioScrobbler *scrob)
						{
							return scrob->SupportsFeature (Media::IAudioScrobbler::Feature::Backdating);
						});

		const auto dia = new TracksSelectorDialog { tracks, scrobblers };
		dia->setAttribute (Qt::WA_DeleteOnClose);

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[dia, scrobblers]
			{
				QHash<Media::IAudioScrobbler*, Media::IAudioScrobbler::BackdatedTracks_t> scrob2tracks;

				for (const auto& track : dia->GetSelectedTracks ())
					for (const auto& pair : Util::Views::Zip (scrobblers, track.Scrobbles_))
						if (pair.second)
							scrob2tracks [pair.first] << track.Track_;

				for (const auto& pair : Util::Stlize (scrob2tracks))
					pair.first->SendBackdated (pair.second);
			},
			dia,
			SIGNAL (accepted ()),
			dia
		};

		connect (dia,
				SIGNAL (finished (int)),
				this,
				SLOT (deleteLater ()));
	}
}
}
}
