/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/


#include "operaimportselectpage.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QTemporaryFile>
#include <QXmlStreamWriter>
#include <util/xpc/util.h>

namespace LC
{
namespace NewLife
{
namespace Importers
{
	OperaImportSelectPage::OperaImportSelectPage (const ICoreProxy_ptr& proxy, QWidget *parent)
	: EntityGeneratingPage { proxy, parent }
	{
		Ui_.setupUi (this);
	}

	int OperaImportSelectPage::nextId () const
	{
		return -1;
	}

	void OperaImportSelectPage::initializePage ()
	{
		connect (wizard (),
				SIGNAL (accepted ()),
				this,
				SLOT (handleAccepted ()));
	}

	void OperaImportSelectPage::checkImportDataAvailable (int index)
	{
		Ui_.HistoryImport_->setChecked (false);
		Ui_.BookmarksImport_->setChecked (false);
		Ui_.RSSImport_->setChecked (false);

		if (!index)
		{
			Ui_.HistoryImport_->setEnabled (false);
			Ui_.BookmarksImport_->setEnabled (false);
			Ui_.RSSImport_->setEnabled (false);
		}
	}

	QList<QVariant> OperaImportSelectPage::GetHistory ()
	{
		QList<QVariant> history;
		QStringList historyPaths;
#ifdef Q_OS_LINUX
		historyPaths << QDir::homePath () + "/.opera/global_history.dat"
				<< QDir::homePath () + "/.opera-next/global_history.dat";
#elif defined Q_OS_WIN
		historyPaths << QDir::homePath () + "/Application Data/Opera Software/Opera Stable/global_history.dat";
#endif
		for (const auto& path : historyPaths)
		{
			QFile oldHistoryFile (path);
			if (!oldHistoryFile.exists () || !oldHistoryFile.open (QIODevice::ReadOnly))
				continue;

			while (!oldHistoryFile.atEnd ())
			{
				const auto& line = oldHistoryFile.readLine ().trimmed ();
				if (line == "-1")
				{
					QMap<QString, QVariant> record;
					record ["Title"] = oldHistoryFile.readLine ().trimmed ();
					record ["URL"] = oldHistoryFile.readLine ().trimmed ();
					record ["DateTime"] = QDateTime::fromSecsSinceEpoch (oldHistoryFile.readLine ()
							.trimmed ().toLongLong ());
					history.push_back (record);
				}
			}
		}

		{
			QString historyDbPath;
#ifdef Q_OS_WIN
			historyDbPath = QDir::homePath () + "/Application Data/Opera Software/Opera Stable/History";
#endif
			if (historyDbPath.isEmpty () || !QFileInfo (historyDbPath).exists ())
				return history;

			QSqlDatabase db = QSqlDatabase::addDatabase ("QSQLITE", "Import history connection");
			db.setDatabaseName (historyDbPath);
			if (!db.open ())
			{
				qWarning () << Q_FUNC_INFO
						<< "could not open database. Try to close Opera"
						<< db.lastError ().text ();
				SendEntity (Util::MakeNotification (tr ("Opera Import"),
						tr ("Could not open Opera database: %1.")
							.arg (db.lastError ().text ()),
						Priority::Critical));
			}
			else
			{
				QSqlQuery query (db);
				query.exec ("SELECT url, title, last_visit_time FROM urls;");
				while (query.next ())
				{
					QMap<QString, QVariant> record;
					record ["URL"] = query.value (0).toUrl ();
					record ["Title"] = query.value (1).toString ();
					record ["DateTime"] = QDateTime::fromSecsSinceEpoch (query.value (2).toLongLong () / 1000000 - 11644473600);
					history.push_back (record);
				}
			}

			QSqlDatabase::database ("Import history connection").close ();
			QSqlDatabase::removeDatabase ("Import history connection");
		}

		return history;
	}

	QList<QVariant> OperaImportSelectPage::GetBookmarks ()
	{
		QList<QVariant> bookmarks;
		QStringList bookmarksPaths;
#ifdef Q_OS_LINUX
		bookmarksPaths << QDir::homePath () + "/.opera/bookmarks.adr"
				<< QDir::homePath () + "/.opera-next/bookmarks.adr";
#elif defined Q_OS_WIN
		bookmarksPaths << QDir::homePath () + "/Application Data/Opera Software/Opera Stable/bookmarks.adr";
#endif
		for (const auto& path : bookmarksPaths)
		{
			QFile oldBookmarksFile (path);
			if (!oldBookmarksFile.exists () || !oldBookmarksFile.open (QIODevice::ReadOnly))
				continue;

			while (!oldBookmarksFile.atEnd ())
			{
				const auto& line = oldBookmarksFile.readLine ().trimmed ();
				if (line == "#URL")
				{
					QMap<QString, QVariant> record;
					oldBookmarksFile.readLine ();
					const auto& nameLine = QString::fromUtf8 (oldBookmarksFile.readLine ().trimmed ());
					record ["Title"] = nameLine.mid (nameLine.indexOf ('=') + 1);
					const auto& urlLine = QString::fromUtf8 (oldBookmarksFile.readLine ().trimmed ());
					record ["URL"] = urlLine.mid (urlLine.indexOf ('=') + 1);
					bookmarks.push_back (record);
				}
			}
		}

		{
			QString bookmarksDbPath;
#ifdef Q_OS_WIN
			bookmarksDbPath = QDir::homePath () + "/Application Data/Opera Software/Opera Stable/favorites.db";
#endif

			if (bookmarksDbPath.isEmpty () || !QFileInfo (bookmarksDbPath).exists ())
				return bookmarks;

			QSqlDatabase db = QSqlDatabase::addDatabase ("QSQLITE", "Import bookmarks connection");
			db.setDatabaseName (bookmarksDbPath);
			if (!db.open ())
			{
				qWarning () << Q_FUNC_INFO
						<< "could not open database. Try to close Opera"
						<< db.lastError ().text ();
				SendEntity (Util::MakeNotification (tr ("Opera Import"),
						tr ("Could not open Opera database: %1.")
							.arg (db.lastError ().text ()),
						Priority::Critical));
			}
			else
			{
				QSqlQuery query (db);
				query.exec ("SELECT name, url FROM favorites;");
				while (query.next ())
				{
					QMap<QString, QVariant> record;
					record ["Title"] = query.value (0).toString ();
					record ["URL"] = query.value (1).toUrl ();
					bookmarks.push_back (record);
				}
			}

			QSqlDatabase::database ("Import bookmarks connection").close ();
			QSqlDatabase::removeDatabase ("Import bookmarks connection");
		}

		return bookmarks;
	}

	QString OperaImportSelectPage::GetImportOpmlFile ()
	{
		QSettings index (QDir::homePath () + "/.opera/mail/index.ini",
				QSettings::IniFormat);
		if (index.status () != QSettings::NoError)
			return QString ();

		QVariantList opmlData;
		for (const auto& group : index.childGroups ())
		{
			if (index.contains (group + "/Type") &&
					index.value (group + "/Type").toInt () == 5)
			{
				QVariantMap opmlLine;
				opmlLine ["Title"] = index.value (group + "/Name").toString ();
				opmlLine ["FeedUrl"] = index.value (group + "/Search Text").toUrl ();
				opmlData << opmlLine;
			}
		}

		if (opmlData.isEmpty ())
			return QString ();

		QTemporaryFile file ("opera_XXXXXX.ompl");
		file.setAutoRemove (false);
		if (!file.open ())
		{
			SendEntity (Util::MakeNotification ("Opera Import",
					tr ("OPML file for importing RSS cannot be created: %1")
							.arg (file.errorString ()),
					Priority::Critical));
			return  QString ();
		}

		QXmlStreamWriter streamWriter (&file);
		streamWriter.setAutoFormatting (true);
		streamWriter.writeStartDocument ();
		streamWriter.writeStartElement ("opml");
		streamWriter.writeAttribute ("version", "1.0");
		streamWriter.writeStartElement ("head");
		streamWriter.writeStartElement ("text");
		streamWriter.writeEndElement ();
		streamWriter.writeEndElement ();
		streamWriter.writeStartElement ("body");
		streamWriter.writeStartElement ("outline");
		streamWriter.writeAttribute ("text", "Live Bookmarks");
		for (const auto& line : opmlData)
		{
			streamWriter.writeStartElement ("outline");
			const auto& opmlLine = line.toMap ();
			QXmlStreamAttributes attr;
			attr.append ("title", opmlLine ["Title"].toString ());
			attr.append ("xmlUrl", opmlLine ["FeedUrl"].toString ());
			attr.append ("text", opmlLine ["Title"].toString ());
			streamWriter.writeAttributes (attr);
			streamWriter.writeEndElement ();
		}
		streamWriter.writeEndElement ();
		streamWriter.writeEndElement ();
		streamWriter.writeEndDocument ();

		QString filename = file.fileName ();
		file.close ();
		return filename;
	}

	void OperaImportSelectPage::handleAccepted ()
	{
		if (Ui_.HistoryImport_->isEnabled () && Ui_.HistoryImport_->isChecked ())
		{
			Entity eHistory = Util::MakeEntity (QVariant (),
				QString (),
				FromUserInitiated,
				"x-leechcraft/browser-import-data");

			eHistory.Additional_ ["BrowserHistory"] = GetHistory ();
			SendEntity (eHistory);
		}

		if (Ui_.BookmarksImport_->isEnabled () && Ui_.BookmarksImport_->isChecked ())
		{
			Entity eBookmarks = Util::MakeEntity (QVariant (),
					QString (),
					FromUserInitiated,
					"x-leechcraft/browser-import-data");

			eBookmarks.Additional_ ["BrowserBookmarks"] = GetBookmarks ();
			SendEntity (eBookmarks);
		}

		if (Ui_.RSSImport_->isEnabled () && Ui_.RSSImport_->isChecked ())
		{
			const auto& file = GetImportOpmlFile ();
			if (file.isEmpty ())
				return;

			Entity eRss = Util::MakeEntity (QUrl::fromLocalFile (file),
					QString (),
					FromUserInitiated,
					"text/x-opml");

			eRss.Additional_ ["RemoveAfterHandling"] = true;
			SendEntity (eRss);
		}
	}
}
}
}
