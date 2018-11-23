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

#include "exportutils.h"
#include <algorithm>
#include <QByteArray>
#include <QDataStream>
#include <QMessageBox>
#include "core.h"
#include "export.h"
#include "opmlwriter.h"
#include "storagebackendmanager.h"
#include "storagebackend.h"

namespace LeechCraft::Aggregator::ExportUtils
{
	namespace
	{
		auto FilterChannels (channels_shorts_t channels, const QSet<IDType_t>& selected)
		{
			const auto removedPos = std::remove_if (channels.begin (), channels.end (),
					[&selected] (const ChannelShort& ch) { return !selected.contains (ch.ChannelID_); });
			channels.erase (removedPos, channels.end ());
			return channels;
		}
	}

	void RunExportOPML (QWidget *parent)
	{
		Export exportDialog (QObject::tr ("Export to OPML"),
				QObject::tr ("Select save file"),
				QObject::tr ("OPML files (*.opml);;"
					"XML files (*.xml);;"
					"All files (*.*)"),
				parent);
		exportDialog.SetFeeds (Core::Instance ().GetChannels ());
		if (exportDialog.exec () == QDialog::Rejected)
			return;

		auto channels = FilterChannels (Core::Instance ().GetChannels (), exportDialog.GetSelectedFeeds ());

		OPMLWriter writer;
		auto data = writer.Write (channels,
				exportDialog.GetTitle (), exportDialog.GetOwner (), exportDialog.GetOwnerEmail ());

		QFile f { exportDialog.GetDestination () };
		if (!f.open (QIODevice::WriteOnly))
		{
			QMessageBox::critical (parent,
					QObject::tr ("OPML export error"),
					QObject::tr ("Could not open file %1 for write.")
							.arg (f.fileName ()));
			return;
		}
		f.write (data.toUtf8 ());
	}

	void RunExportBinary (QWidget *parent)
	{
		Export exportDialog (QObject::tr ("Export to binary file"),
				QObject::tr ("Select save file"),
				QObject::tr ("Aggregator exchange files (*.lcae);;"
					"All files (*.*)"),
				parent);
		exportDialog.SetFeeds (Core::Instance ().GetChannels ());
		if (exportDialog.exec () == QDialog::Rejected)
			return;

		auto channels = FilterChannels (Core::Instance ().GetChannels (), exportDialog.GetSelectedFeeds ());

		QFile f { exportDialog.GetDestination () };
		if (!f.open (QIODevice::WriteOnly))
		{
			QMessageBox::critical (parent,
					QObject::tr ("Binary export error"),
					QObject::tr ("Could not open file %1 for write.")
						.arg (f.fileName ()));
			return;
		}

		QByteArray buffer;
		QDataStream data (&buffer, QIODevice::WriteOnly);

		int version = 1;
		int magic = 0xd34df00d;
		data << magic
				<< version
				<< exportDialog.GetTitle ()
				<< exportDialog.GetOwner ()
				<< exportDialog.GetOwnerEmail ();

		const auto sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();
		for (const auto& cs : channels)
		{
			auto channel = sb->GetChannel (cs.ChannelID_);
			channel.Items_ = sb->GetFullItems (channel.ChannelID_);
			data << channel;
		}

		f.write (qCompress (buffer, 9));
	}
}