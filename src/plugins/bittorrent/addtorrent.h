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

#include <QDialog>
#include <QVector>
#include "ui_addtorrent.h"
#include "core.h"

namespace LeechCraft
{
namespace Plugins
{
namespace BitTorrent
{
	class AddTorrentFilesModel;

	class AddTorrent : public QDialog
	{
		Q_OBJECT

		Ui::AddTorrent Ui_;

		AddTorrentFilesModel * const FilesModel_;
	public:
		AddTorrent (QWidget *parent = 0);
		void Reinit ();
		void SetFilename (const QString&);
		void SetSavePath (const QString&);
		QString GetFilename () const;
		QString GetSavePath () const;
		bool GetTryLive () const;
		QVector<bool> GetSelectedFiles () const;
		Core::AddType GetAddType () const;
		void SetTags (const QStringList& tags);
		QStringList GetTags () const;
		Util::TagsLineEdit* GetEdit ();
	private slots:
		void on_TorrentBrowse__released ();
		void on_DestinationBrowse__released ();
		void on_MarkAll__released ();
		void on_UnmarkAll__released ();
		void on_MarkSelected__released ();
		void on_UnmarkSelected__released ();
		void setOkEnabled ();
		void updateAvailableSpace ();
	private:
		void ParseBrowsed ();
		QPair<quint64, quint64> GetAvailableSpaceInDestination ();
	signals:
		void on_TorrentFile__textChanged ();
		void on_Destination__textChanged ();
	};
}
}
}
