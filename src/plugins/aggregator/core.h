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

#include <memory>
#include <QAbstractItemModel>
#include <QString>
#include <QMap>
#include <QPair>
#include <QList>
#include <QDateTime>
#include <interfaces/idownload.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ihookproxy.h>
#include <util/idpool.h>
#include "item.h"
#include "channel.h"
#include "feed.h"
#include "storagebackend.h"
#include "actionsstructs.h"
#include "dbupdatethreadfwd.h"

class QTimer;
class QNetworkReply;
class QFile;
class QSortFilterProxyModel;
class QToolBar;
class IWebBrowser;

namespace LeechCraft
{
namespace Util
{
	class ShortcutManager;
}

namespace Aggregator
{
	class ChannelsModel;
	class JobHolderRepresentation;
	class ChannelsFilterModel;
	class PluginManager;

	class Core : public QObject
	{
		Q_OBJECT

		enum Columns
		{
			ColumnName = 0
			, ColumnDate = 1
		};

		struct PendingOPML
		{
			QString Filename_;
		};
		QMap<int, PendingOPML> PendingOPMLs_;

		struct PendingJob
		{
			enum Role
			{
				RFeedAdded
				, RFeedUpdated
				, RFeedExternalData
			} Role_;
			QString URL_;
			QString Filename_;
			QStringList Tags_;
			std::shared_ptr<Feed::FeedSettings> FeedSettings_;
		};
		struct ExternalData
		{
			enum Type
			{
				TImage
				, TIcon
			} Type_;
			Channel_ptr RelatedChannel_;
		};
		QMap<int, PendingJob> PendingJobs_;
		QMap<QString, ExternalData> PendingJob2ExternalData_;
		QList<QObject*> Downloaders_;
		QMap<int, QObject*> ID2Downloader_;

		ChannelsModel *ChannelsModel_ = nullptr;
		QTimer *UpdateTimer_ = nullptr, *CustomUpdateTimer_ = nullptr;
		std::shared_ptr<StorageBackend> StorageBackend_;
		JobHolderRepresentation *JobHolderRepresentation_ = nullptr;
		QMap<IDType_t, QDateTime> Updates_;
		ChannelsFilterModel *ChannelsFilterModel_ = nullptr;
		ICoreProxy_ptr Proxy_;
		bool Initialized_ = false;
		AppWideActions AppWideActions_;

		QList<IDType_t> UpdatesQueue_;

		PluginManager *PluginManager_ = nullptr;

		std::shared_ptr<DBUpdateThread> DBUpThread_;

		Util::ShortcutManager *ShortcutMgr_ = nullptr;

		Core ();
	private:
		QHash<PoolType, Util::IDPool<IDType_t>> Pools_;
	public:
		struct ChannelInfo
		{
			IDType_t FeedID_;
			IDType_t ChannelID_;
			QString URL_;
			QString Link_;
			QString Description_;
			QString Author_;
			int NumItems_;
		};

		static Core& Instance ();
		void Release ();

		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;

		void AddPlugin (QObject*);

		Util::IDPool<IDType_t>& GetPool (PoolType);

		bool CouldHandle (const LeechCraft::Entity&);
		void Handle (LeechCraft::Entity);
		void StartAddingOPML (const QString&);
		void SetAppWideActions (const AppWideActions&);
		const AppWideActions& GetAppWideActions () const;

		bool DoDelayedInit ();
		bool ReinitStorage ();

		void AddFeed (const QString&, const QString&);
		void AddFeed (QString, const QStringList&,
				FeedSettings_ptr = FeedSettings_ptr ());
		void RemoveFeed (const QModelIndex&);
		void RenameFeed (const QModelIndex& index, const QString& newName);
		void RemoveChannel (const QModelIndex&);

		Util::ShortcutManager* GetShortcutManager () const;

		/** Returns the channels model as it is.
			*
			* @sa GetRawChannelsModel
			*/
		ChannelsModel* GetRawChannelsModel () const;

		/** Returns the filter model with the
			* GetRawChannelsModel as a source.
			*
			* @sa GetRawChannelsModel.
			*/
		QSortFilterProxyModel* GetChannelsModel () const;
		IWebBrowser* GetWebBrowser () const;
		void MarkChannelAsRead (const QModelIndex&);
		void MarkChannelAsUnread (const QModelIndex&);

		ChannelInfo GetChannelInfo (const QModelIndex&) const;
		QPixmap GetChannelPixmap (const QModelIndex&) const;

		/** Sets the tags for index from the given user-edited string.
			*/
		void SetTagsForIndex (const QString&, const QModelIndex&);
		void UpdateFavicon (const QModelIndex&);

		QStringList GetCategories (const QModelIndex&) const;
		QStringList GetCategories (const items_shorts_t&) const;

		Feed::FeedSettings GetFeedSettings (const QModelIndex&) const;
		void SetFeedSettings (const Feed::FeedSettings&);
		void UpdateFeed (const QModelIndex&);
		void AddFromOPML (const QString&,
				const QString&,
				const std::vector<bool>&);
		void ExportToOPML (const QString&,
				const QString&,
				const QString&,
				const QString&,
				const std::vector<bool>&) const;
		void ExportToBinary (const QString&,
				const QString&,
				const QString&,
				const QString&,
				const std::vector<bool>&) const;
		JobHolderRepresentation* GetJobHolderRepresentation () const;

		void GetChannels (channels_shorts_t&) const;
		void AddFeeds (const feeds_container_t&, const QString&);
	public slots:
		void openLink (const QString&);
		void updateFeeds ();
		void updateIntervalChanged ();
		void handleSslError (QNetworkReply*);
	private slots:
		void fetchExternalFile (const QString&, const QString&);
		void handleJobFinished (int);
		void handleJobRemoved (int);
		void handleJobError (int, IDownload::Error);
		void handleCustomUpdates ();
		void rotateUpdatesQueue ();
	private:
		void FetchPixmap (const Channel_ptr&);
		void FetchFavicon (const Channel_ptr&);
		void HandleExternalData (const QString&, const QFile&);
		void HandleFeedAdded (const channels_container_t&,
				const PendingJob&);
		void HandleFeedUpdated (const channels_container_t&,
				const PendingJob&);
		void MarkChannel (const QModelIndex&, bool);
		void UpdateFeed (const IDType_t&);
		void HandleProvider (QObject*, int);
		void ErrorNotification (const QString&, const QString&, bool = true) const;
	signals:
		void storageChanged ();

		// Plugin API
		void hookGotNewItems (LeechCraft::IHookProxy_ptr proxy,
				const QList<Item_cptr>& items);
	};
}
}
