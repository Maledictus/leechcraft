/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "foldersmodel.h"
#include <algorithm>
#include <QIcon>
#include <QFont>
#include <QMimeData>
#include <QtDebug>
#include "account.h"
#include "core.h"
#include "storage.h"
#include "folder.h"
#include "vmimeconversions.h"

namespace LC
{
namespace Snails
{
	struct FolderDescr
	{
		QString Name_;

		std::weak_ptr<FolderDescr> Parent_;
		QList<FolderDescr_ptr> Children_;

		FolderType Type_ = FolderType::Other;

		int MessageCount_ = -1;
		int UnreadCount_ = -1;

		FolderDescr () = default;
		FolderDescr (const QString& name, const std::weak_ptr<FolderDescr>& parent);

		QList<FolderDescr_ptr>::iterator Find (const QString& name);

		std::ptrdiff_t Row () const;
	};

	FolderDescr::FolderDescr (const QString& name, const std::weak_ptr<FolderDescr>& parent)
	: Name_ { name }
	, Parent_ { parent }
	{
	}

	QList<FolderDescr_ptr>::iterator FolderDescr::Find (const QString& name)
	{
		return std::find_if (Children_.begin (), Children_.end (),
				[&name] (const FolderDescr_ptr& descr) { return descr->Name_ == name; });
	}

	std::ptrdiff_t FolderDescr::Row () const
	{
		const auto& parentParent = Parent_.lock ();
		const auto parentPos = std::find_if (parentParent->Children_.begin (),
				parentParent->Children_.end (),
				[this] (const FolderDescr_ptr& folder) { return folder.get () == this; });
		if (parentPos == parentParent->Children_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "could not determine parent's folder";
			return -1;
		}

		return std::distance (parentParent->Children_.begin (), parentPos);
	}

	FoldersModel::FoldersModel (Account *acc)
	: QAbstractItemModel { acc }
	, Acc_ { acc }
	, Headers_ { tr ("Folder"), tr ("Messages") }
	, RootFolder_ { new FolderDescr {} }
	{
	}

	QVariant FoldersModel::headerData (int section, Qt::Orientation orient, int role) const
	{
		if (orient != Qt::Horizontal || role != Qt::DisplayRole)
			return {};

		return Headers_.value (section);
	}

	int FoldersModel::columnCount (const QModelIndex&) const
	{
		return Headers_.size ();
	}

	QVariant FoldersModel::data (const QModelIndex& index, int role) const
	{
		const auto folder = static_cast<FolderDescr*> (index.internalPointer ());

		switch (role)
		{
		case Qt::DisplayRole:
			break;
		case Qt::DecorationRole:
			if (index.column ())
				return {};

			return GetFolderIcon (folder->Type_);
		case Qt::FontRole:
		{
			QFont font;
			font.setBold (folder->UnreadCount_ > 0);
			return font;
		}
		case Role::FolderPath:
		{
			QStringList path { folder->Name_ };
			auto wItem = folder->Parent_;
			while (wItem.lock () != RootFolder_)
			{
				const auto& item = wItem.lock ();
				path.prepend (item->Name_);
				wItem = item->Parent_;
			}
			return path;
		}
		default:
			return {};
		}

		switch (index.column ())
		{
		case Column::FolderName:
			return folder->Name_;
		case Column::MessageCount:
		{
			if (folder->MessageCount_ < 0)
				return {};

			auto result = QString::number (folder->MessageCount_);
			if (folder->UnreadCount_)
				result += " (" + QString::number (folder->UnreadCount_) + ")";
			return result;
		}
		default:
			return {};
		}
	}

	Qt::ItemFlags FoldersModel::flags (const QModelIndex& index) const
	{
		auto flags = QAbstractItemModel::flags (index);
		if (index.isValid ())
			flags |= Qt::ItemIsDropEnabled;
		return flags;
	}

	QModelIndex FoldersModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		const auto folder = parent.isValid () ?
				static_cast<FolderDescr*> (parent.internalPointer ()) :
				RootFolder_.get ();

		return createIndex (row, column, folder->Children_.value (row).get ());
	}

	QModelIndex FoldersModel::parent (const QModelIndex& index) const
	{
		const auto folder = static_cast<FolderDescr*> (index.internalPointer ());
		const auto parentFolder = folder->Parent_.lock ().get ();
		if (parentFolder == RootFolder_.get ())
			return {};

		const auto row = parentFolder->Row ();
		if (row < 0)
			return {};

		return createIndex (row, 0, parentFolder);
	}

	int FoldersModel::rowCount (const QModelIndex& parent) const
	{
		const auto folder = parent.isValid () ?
				static_cast<FolderDescr*> (parent.internalPointer ()) :
				RootFolder_.get ();
		return folder->Children_.size ();
	}

	QStringList FoldersModel::mimeTypes () const
	{
		return
		{
			Mimes::FolderPath,
			Mimes::MessageIdList
		};
	}

	bool FoldersModel::canDropMimeData (const QMimeData *data, Qt::DropAction action,
			int, int, const QModelIndex&) const
	{
		if (!(action == Qt::MoveAction || action == Qt::CopyAction))
			return false;

		return data->hasFormat (Mimes::FolderPath) &&
				data->hasFormat (Mimes::MessageIdList);
	}

	bool FoldersModel::dropMimeData (const QMimeData *data, Qt::DropAction action,
			int, int, const QModelIndex& parent)
	{
		const auto& srcFolder = QString::fromUtf8 (data->data (Mimes::FolderPath)).split ('/');

		const auto& ids = [data]
		{
			QList<QByteArray> ids;
			QDataStream istr { data->data (Mimes::MessageIdList) };
			istr >> ids;
			return ids;
		} ();

		if (ids.isEmpty () || srcFolder.isEmpty ())
			return false;

		emit msgMoveRequested (ids, srcFolder, parent.data (Role::FolderPath).toStringList (), action);
		return true;
	}

	Qt::DropActions FoldersModel::supportedDropActions () const
	{
		return Qt::MoveAction | Qt::CopyAction;
	}

	void FoldersModel::SetFolders (const QList<Folder>& folders)
	{
		for (auto& custom : CustomFolders_)
			custom.clear ();

		if (const auto rc = RootFolder_->Children_.size ())
		{
			Folder2Descr_.clear ();

			beginRemoveRows ({}, 0, rc - 1);
			RootFolder_->Children_.clear ();
			endRemoveRows ();
		}

		auto newRoot = std::make_shared<FolderDescr> ();
		for (const auto& folder : folders)
		{
			auto currentRoot = newRoot;
			for (const auto& component : folder.Path_)
			{
				const auto componentDescrPos = currentRoot->Find (component);
				if (componentDescrPos != currentRoot->Children_.end ())
				{
					currentRoot = *componentDescrPos;
					continue;
				}

				const auto& componentDescr = std::make_shared<FolderDescr> (component, currentRoot);
				currentRoot->Children_.append (componentDescr);
				currentRoot = componentDescr;
			}
			currentRoot->Type_ = folder.Type_;

			Folder2Descr_ [folder.Path_] = currentRoot.get ();

			if (folder.Type_ != FolderType::Other)
				CustomFolders_ [static_cast<int> (folder.Type_)] = folder.Path_;
		}

		if (const auto newRc = newRoot->Children_.size ())
		{
			beginInsertRows ({}, 0, newRc - 1);
			RootFolder_ = newRoot;
			endInsertRows ();
		}
	}

	void FoldersModel::SetFolderCounts (const QStringList& folder, int unread, int total)
	{
		const auto descr = Folder2Descr_.value (folder);
		if (!descr)
		{
			qWarning () << Q_FUNC_INFO
					<< "no description for folder"
					<< folder;
			return;
		}

		descr->UnreadCount_ = unread;
		descr->MessageCount_ = total;

		const auto& first = createIndex (descr->Row (), 0, descr);
		const auto& second = createIndex (descr->Row (), columnCount () - 1, descr);
		emit dataChanged (first, second);
	}

	std::optional<QStringList> FoldersModel::GetFolderPath (FolderType type) const
	{
		if (type == FolderType::Other)
			return {};

		const auto& value = CustomFolders_ [static_cast<int> (type)];
		if (!value.isEmpty ())
			return value;
		else
			return {};
	}
}
}
