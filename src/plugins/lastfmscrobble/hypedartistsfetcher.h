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
#include <QFutureInterface>
#include <util/sll/either.h>
#include <interfaces/media/ihypesprovider.h>

class QNetworkAccessManager;

namespace LC
{
namespace Lastfmscrobble
{
	class HypedArtistsFetcher : public QObject
	{
		QNetworkAccessManager *NAM_;
		QList<Media::HypedArtistInfo> Infos_;

		int InfoCount_ = 0;

		QFutureInterface<Media::IHypesProvider::HypeQueryResult_t> Promise_;
	public:
		HypedArtistsFetcher (QNetworkAccessManager*, Media::IHypesProvider::HypeType, QObject* = 0);

		QFuture<Media::IHypesProvider::HypeQueryResult_t> GetFuture ();
	private:
		void DecrementWaiting ();
		void HandleFinished (const QByteArray&);
	};
}
}
