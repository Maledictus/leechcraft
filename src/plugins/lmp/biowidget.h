/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QWidget>
#include "ui_biowidget.h"

class QQuickWidget;

namespace Media
{
	class IArtistBioFetcher;
}

namespace LC
{
namespace LMP
{
	class BioViewManager;

	class BioWidget : public QWidget
	{
		Q_OBJECT

		Ui::BioWidget Ui_;

		QQuickWidget * const View_;

		BioViewManager *Manager_;

		QList<Media::IArtistBioFetcher*> Providers_;

		struct Current
		{
			QString Artist_;
			QStringList Hints_;
		} Current_;
	public:
		BioWidget (QWidget* = 0);

		void SetCurrentArtist (const QString&, const QStringList&);
	private slots:
		void saveLastUsedProv ();
		void requestBiography ();
	signals:
		void gotArtistImage (const QString&, const QUrl&);
	};
}
}
