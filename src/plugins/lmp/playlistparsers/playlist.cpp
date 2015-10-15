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

#include "playlist.h"
#include <algorithm>
#include <util/sll/qtutil.h>
#include <util/util.h>
#include "mediainfo.h"

namespace LeechCraft
{
namespace LMP
{
	PlaylistItem::PlaylistItem (const AudioSource& source)
	: Source_ { source }
	{
	}

	PlaylistItem::PlaylistItem (const AudioSource& source, const QVariantMap& additional)
	: Source_ { source }
	, Additional_ { additional }
	{
	}

	namespace
	{
		QVariantMap FromMediaInfo (const MediaInfo& info)
		{
			QVariantMap result;

			for (const auto& pair : Util::Stlize (info.Additional_))
				if (pair.second.canConvert<QString> ())
					result [pair.first] = pair.second;

			result.unite (Util::MakeMap<QString, QVariant> ({
						{ "LMP/HasMediaInfo", true },
						{ "LMP/Artist", info.Artist_ },
						{ "LMP/Album", info.Album_ },
						{ "LMP/Title", info.Title_ },
						{ "LMP/Genres", info.Genres_.join (" / ") },
						{ "LMP/Length", info.Length_ },
						{ "LMP/Year", info.Year_ },
						{ "LMP/TrackNumber", info.TrackNumber_ }
					}));

			return result;
		}
	}

	Playlist::Playlist (const QList<PlaylistItem>& items)
	: Playlist_ { items }
	{
	}

	Playlist::Playlist (const QList<AudioSource>& sources)
	{
		Playlist_.reserve (sources.size ());
		for (const auto& src : sources)
			Playlist_.append (PlaylistItem { src });
	}

	Playlist::const_iterator Playlist::begin () const
	{
		return Playlist_.begin ();
	}

	Playlist::iterator Playlist::begin ()
	{
		return Playlist_.begin ();
	}

	Playlist::const_iterator Playlist::end () const
	{
		return Playlist_.end ();
	}

	Playlist::iterator Playlist::end ()
	{
		return Playlist_.end ();
	}

	Playlist::iterator Playlist::erase (iterator it)
	{
		return Playlist_.erase (it);
	}

	Playlist& Playlist::Append (const PlaylistItem& item)
	{
		if (UrlsSet_.contains (item.Source_.ToUrl ()))
			return *this;

		Playlist_ << item;
		UrlsSet_ << item.Source_.ToUrl ();
		return *this;
	}

	Playlist& Playlist::operator+= (const AudioSource& src)
	{
		Append (PlaylistItem { src });
		return *this;
	}

	Playlist& Playlist::operator+= (const Playlist& playlist)
	{
		for (const auto& item : playlist.Playlist_)
			Append (item);
		return *this;
	}

	QList<AudioSource> Playlist::ToSources () const
	{
		QList<AudioSource> result;
		result.reserve (Playlist_.size ());
		for (const auto& item : Playlist_)
			result << item.Source_;
		return result;
	}

	bool Playlist::IsEmpty () const
	{
		return Playlist_.isEmpty ();
	}

	bool Playlist::SetProperty (const AudioSource& src, const QString& key, const QVariant& value)
	{
		const auto srcPos = std::find_if (Playlist_.begin (), Playlist_.end (),
				[&src] (const PlaylistItem& item) { return item.Source_ == src; });
		if (srcPos == Playlist_.end ())
			return false;

		srcPos->Additional_ [key] = value;
		return true;
	}
}
}
