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

#include <QAbstractItemModel>
#include <QStringList>
#include <QList>
#include <interfaces/iinfo.h>
#include <interfaces/core/ihookproxy.h>

namespace LeechCraft
{
namespace Poshuku
{
	class FavoritesModel : public QAbstractItemModel
	{
		Q_OBJECT

		QStringList ItemHeaders_;
	public:
		struct FavoritesItem
		{
			QString Title_;
			QString URL_;
			/// Contains ids of the real tags.
			QStringList Tags_;

			bool operator== (const FavoritesItem&) const;
		};
		typedef QList<FavoritesItem> items_t;
	private:
		items_t Items_;
		QMap<QString, QString> CheckResults_;
	public:
		enum Columns
		{
			ColumnTitle
			, ColumnURL
			, ColumnTags
		};

		FavoritesModel (QObject* = nullptr);

		void HandleStorageReady ();

		int columnCount (const QModelIndex& = QModelIndex ()) const;
		QVariant data (const QModelIndex&, int = Qt::DisplayRole) const;
		Qt::ItemFlags flags (const QModelIndex&) const;
		QVariant headerData (int, Qt::Orientation, int = Qt::DisplayRole) const;
		QModelIndex index (int, int, const QModelIndex& = QModelIndex()) const;
		QModelIndex parent (const QModelIndex&) const;
		int rowCount (const QModelIndex& = QModelIndex ()) const;
		bool setData (const QModelIndex&, const QVariant&, int = Qt::EditRole);

		Qt::DropActions supportedDropActions () const;
		QStringList mimeTypes () const;
		QMimeData* mimeData (const QModelIndexList& indexes) const;
		bool dropMimeData (const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

		void EditBookmark (const QModelIndex&);
		void ChangeURL (const QModelIndex&, const QString&);
		const items_t& GetItems () const;
		void SetCheckResults (const QMap<QString, QString>&);

		bool IsUrlExists (const QString&) const;
	private:
		QStringList GetVisibleTags (int) const;
		FavoritesItem GetItemFromUrl (const QString& url);
	public slots:
		QModelIndex addItem (const QString&, const QString&, const QStringList&);
		QList<QVariant> getItemsMap () const;
		void removeItem (const QModelIndex&);
		void removeItem (const QString&);
		void handleItemAdded (const FavoritesModel::FavoritesItem&);
		void handleItemUpdated (const FavoritesModel::FavoritesItem&);
		void handleItemRemoved (const FavoritesModel::FavoritesItem&);
	private slots:
		void loadData ();
	signals:
		void error (const QString&);

		// Hook support
		void hookAddedToFavorites (LeechCraft::IHookProxy_ptr,
				QString title, QString url, QStringList tags);
	};
}
}
