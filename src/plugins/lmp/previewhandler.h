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

#pragma once

#include <QObject>
#include <QPair>
#include <QHash>
#include <QStringList>

namespace Media
{
	struct AudioSearchRequest;
	class IAudioPile;
	class IPendingAudioSearch;
}

namespace LeechCraft
{
namespace LMP
{
	class Player;

	class PreviewHandler : public QObject
	{
		Q_OBJECT

		Player *Player_;

		QList<Media::IAudioPile*> Providers_;

		QHash<QString, QHash<QString, QHash<QString, int>>> Artist2Album2Tracks_;

		struct PendingTrackInfo
		{
			QString Artist_;
			QString Album_;
			QString Track_;
		};
		QHash<Media::IPendingAudioSearch*, PendingTrackInfo> Pending2Track_;
	public:
		PreviewHandler (Player*, QObject* = nullptr);

		void InitWithPlugins ();
	public slots:
		void previewArtist (const QString& artist);
		void previewTrack (const QString& track, const QString& artist);
		void previewTrack (const QString& track, const QString& artist, int length);
		void previewAlbum (const QString& artist, const QString& album,
				const QList<QPair<QString, int>>& tracks);
	private:
		QList<Media::IPendingAudioSearch*> RequestPreview (const Media::AudioSearchRequest&);
		void CheckPendingAlbum (Media::IPendingAudioSearch*);
	private slots:
		void handlePendingReady ();
	};
}
}
