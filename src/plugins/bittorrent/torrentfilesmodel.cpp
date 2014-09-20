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

#include "torrentfilesmodel.h"
#include <iterator>
#include <boost/functional/hash.hpp>
#include <QUrl>
#include <QTimer>
#include <QtDebug>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sys/extensionsdata.h>
#include <util/models/modelitembase.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"

namespace LeechCraft
{
namespace Plugins
{
namespace BitTorrent
{
	TorrentFilesModel::TorrentFilesModel (int index)
	: TorrentFilesModelBase { { tr ("Name"), tr ("Priority"), tr ("Progress") } }
	, Index_ { index }
	{
		auto timer = new QTimer (this);
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (update ()));
		timer->start (2000);

		QTimer::singleShot (0,
				this,
				SLOT (update ()));
	}

	QVariant TorrentFilesModel::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid ())
			return {};

		const auto node = static_cast<TorrentNodeInfo*> (index.internalPointer ());
		switch (role)
		{
		case Qt::CheckStateRole:
			return {};
		case Qt::DisplayRole:
			switch (index.column ())
			{
			case ColumnPath:
				return node->Name_;
			case ColumnPriority:
				return node->Priority_;
			case ColumnProgress:
				return node->Progress_;
			}
		case Qt::DecorationRole:
			return index.column () == ColumnPath ?
					node->Icon_ :
					QIcon {};
		case RoleFullPath:
			return QString::fromUtf8 (node->GetFullPath ().c_str ());
		case RoleFileName:
			return node->Name_;
		case RoleProgress:
			return std::max ({}, node->Progress_);
		case RoleSize:
			return node->SubtreeSize_;
		case RolePriority:
			return node->Priority_;
		}
		return {};
	}

	Qt::ItemFlags TorrentFilesModel::flags (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return 0;

		if (index.column () == ColumnPriority ||
				index.column () == ColumnPath)
			return Qt::ItemIsSelectable |
				Qt::ItemIsEnabled |
				Qt::ItemIsEditable;
		else
			return Qt::ItemIsSelectable |
				Qt::ItemIsEnabled;
	}

	bool TorrentFilesModel::setData (const QModelIndex& index, const QVariant& value, int role)
	{
		if (!index.isValid () || role != Qt::EditRole)
			return false;

		const auto node = static_cast<TorrentNodeInfo*> (index.internalPointer ());
		if (index.column () == ColumnPriority)
		{
			if (const auto rc = rowCount (index))
				for (int i = 0; i < rc; ++i)
					setData (this->index (i, index.column (), index), value, role);
			else
			{
				const auto newPriority = value.toInt ();
				Core::Instance ()->SetFilePriority (node->FileIndex_, newPriority, Index_);
				node->Priority_ = newPriority;
				emit dataChanged (index, index);
			}
			return true;
		}
		else if (index.column () == ColumnPath)
		{
			Core::Instance ()->SetFilename (node->FileIndex_, value.toString (), Index_);
			return true;
		}
		else
			return false;
	}

	void TorrentFilesModel::ResetFiles (const boost::filesystem::path& basePath,
			const QList<FileInfo>& infos)
	{
		Clear ();

		BasePath_ = basePath;

		beginInsertRows ({}, 0, 0);
		FilesInTorrent_ = infos.size ();
		Path2Node_ [{}] = RootNode_;

		const auto& inst = Util::ExtensionsData::Instance ();

		for (int i = 0; i < infos.size (); ++i)
		{
			const auto& fi = infos.at (i);
			const auto& parentItem = MkParentIfDoesntExist (fi.Path_);

			const auto& filename =
#ifdef Q_OS_WIN32
					QString::fromUtf16 (reinterpret_cast<const ushort*> (fi.Path_.leaf ().c_str ()));
#else
					QString::fromUtf8 (fi.Path_.leaf ().c_str ());
#endif

			const auto item = parentItem->AppendChild (parentItem);
			item->Name_ = filename;
			item->ParentPath_ = fi.Path_.branch_path ();
			item->Priority_ = fi.Priority_;
			item->FileIndex_ = i;
			item->SubtreeSize_ = fi.Size_;
			item->Progress_ = fi.Progress_;
			item->Icon_ = inst.GetExtIcon (filename.section ('.', -1));

			Path2Node_ [fi.Path_] = item;
		}

		UpdateSizeGraph (RootNode_);

		endInsertRows ();
	}

	void TorrentFilesModel::UpdateFiles (const boost::filesystem::path& basePath,
			const QList<FileInfo>& infos)
	{
		BasePath_ = basePath;
		if (Path2Node_.size () <= 1)
		{
			ResetFiles (BasePath_, infos);
			return;
		}

		for (const auto& fi : infos)
		{
			const auto pos = Path2Node_.find (fi.Path_);
			if (pos == Path2Node_.end ())
			{
				Clear ();
				ResetFiles (BasePath_, infos);
				return;
			}

			const auto& item = pos->second;
			item->Progress_ = fi.Progress_;
			item->SubtreeSize_ = fi.Size_;
		}

		UpdateSizeGraph (RootNode_);

		if (const auto rc = RootNode_->GetRowCount ())
			emit dataChanged (index (0, 2), index (rc - 1, 2));
	}

	void TorrentFilesModel::HandleFileActivated (QModelIndex index) const
	{
		if (!index.isValid ())
			return;

		if (index.column () != ColumnPath)
			index = index.sibling (index.row (), ColumnPath);

		const auto item = static_cast<TorrentNodeInfo*> (index.internalPointer ());

		const auto iem = Core::Instance ()->GetProxy ()->GetEntityManager ();
		if (std::abs (item->Progress_ - 1) >= std::numeric_limits<decltype (item->Progress_)>::epsilon ())
			iem->HandleEntity (Util::MakeNotification ("BitTorrent",
					tr ("%1 hasn't finished downloading yet.")
						.arg ("<em>" + item->Name_ + "</em>"),
					PWarning_));
		else
		{
			const auto& full = BasePath_ / item->GetFullPath ();
			const auto& path = QString::fromUtf8 (full.string ().c_str ());
			const auto& e = Util::MakeEntity (QUrl::fromLocalFile (path),
					{},
					FromUserInitiated);
			iem->HandleEntity (e);
		}
	}

	void TorrentFilesModel::update ()
	{
		const auto& handle = Core::Instance ()->GetTorrentHandle (Index_);
		const auto& base = handle.save_path ();

		const auto& files = Core::Instance ()->GetTorrentFiles (Index_);
		UpdateFiles (base, files);
	}

	void TorrentFilesModel::UpdateSizeGraph (const TorrentNodeInfo_ptr& node)
	{
		if (!node->GetRowCount ())
			return;

		qulonglong size = 0;
		qulonglong done = 0;

		for (const auto & child : *node)
		{
			UpdateSizeGraph (child);
			size += child->SubtreeSize_;
			done += child->SubtreeSize_ * child->Progress_;
		}

		node->SubtreeSize_ = size;
		node->Progress_ = size ? static_cast<double> (done) / size : 1;
	}
}
}
}

