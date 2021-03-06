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

#include "importentity.h"
#include <QMainWindow>
#include <QProgressDialog>
#include <interfaces/structures.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include "historymodel.h"
#include "favoritesmodel.h"

namespace LC
{
namespace Poshuku
{
	namespace
	{
		void ImportHistory (const QVariantList& history,
				HistoryModel *historyModel, IRootWindowsManager *rootWM)
		{
			if (history.isEmpty ())
				return;

			QProgressDialog progressDia
			{
				QObject::tr ("Importing history..."),
				QObject::tr ("Abort"),
				0,
				history.size (),
				rootWM->GetPreferredWindow ()
			};
			qDebug () << Q_FUNC_INFO << history.size ();
			for (const QVariant& hRowVar : history)
			{
				const auto& hRow = hRowVar.toMap ();
				const auto& title = hRow ["Title"].toString ();
				const auto& url = hRow ["URL"].toString ();
				const auto& date = hRow ["DateTime"].toDateTime ();

				if (!date.isValid ())
					qWarning () << "skipping entity with invalid date" << title << url;
				else
					historyModel->addItem (title, url, date);

				progressDia.setValue (progressDia.value () + 1);
				if (progressDia.wasCanceled ())
					break;
			}
		}

		void ImportBookmarks (const QVariantList& bookmarks,
				FavoritesModel *favoritesModel, IRootWindowsManager *rootWM)
		{
			if (bookmarks.isEmpty ())
				return;

			QProgressDialog progressDia
			{
				QObject::tr ("Importing bookmarks..."),
				QObject::tr ("Abort"),
				0,
				bookmarks.size (),
				rootWM->GetPreferredWindow ()
			};
			qDebug () << "Bookmarks" << bookmarks.size ();
			for (const auto& hBMVar : bookmarks)
			{
				const auto& hBM = hBMVar.toMap ();
				const auto& title = hBM ["Title"].toString ();
				const auto& url = hBM ["URL"].toString ();
				const auto& tags = hBM ["Tags"].toStringList ();

				favoritesModel->addItem (title, url, tags);

				progressDia.setValue (progressDia.value () + 1);
				if (progressDia.wasCanceled ())
					break;
			}
		}
	}

	void ImportEntity (const Entity& e,
			HistoryModel *historyModel, FavoritesModel *favoritesModel, IRootWindowsManager *rootWM)
	{
		ImportHistory (e.Additional_ ["BrowserHistory"].toList (), historyModel, rootWM);
		ImportBookmarks (e.Additional_ ["BrowserBookmarks"].toList (), favoritesModel, rootWM);
	}
}
}
