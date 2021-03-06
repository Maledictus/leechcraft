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
#include <boost/optional.hpp>
#include <QObject>
#include <interfaces/core/icoreproxy.h>

class QUrl;

namespace LC
{
struct Entity;

namespace LMP
{
	struct MediaInfo;
	class LocalCollection;
	class HookInterconnector;
	class LocalFileResolver;
	class PlaylistManager;
	class SyncManager;
	class SyncUnmountableManager;
	class CloudUploadManager;
	class Player;
	class PreviewHandler;
	class ProgressManager;
	class RadioManager;
	class CollectionsManager;
	class LMPProxy;

	class Core : public QObject
	{
		Q_OBJECT

		static std::shared_ptr<Core> CoreInstance_;

		const ICoreProxy_ptr Proxy_;

		struct Members;

		std::shared_ptr<Members> M_;

		QObjectList SyncPlugins_;
		QObjectList CloudPlugins_;

		Core (const ICoreProxy_ptr&);

		Core () = delete;
		Core (const Core&) = delete;
		Core (Core&&) = delete;
		Core& operator= (const Core&) = delete;
		Core& operator= (Core&&) = delete;
	public:
		static Core& Instance ();
		static void InitWithProxy (const ICoreProxy_ptr&);

		void Release ();

		ICoreProxy_ptr GetProxy ();

		void SendEntity (const Entity&);

		void InitWithOtherPlugins ();

		LMPProxy* GetLmpProxy () const;

		void AddPlugin (QObject*);
		QObjectList GetSyncPlugins () const;
		QObjectList GetCloudStoragePlugins () const;

		void RequestArtistBrowser (const QString&);

		HookInterconnector* GetHookInterconnector () const;
		LocalFileResolver* GetLocalFileResolver () const;
		LocalCollection* GetLocalCollection () const;
		CollectionsManager* GetCollectionsManager () const;
		PlaylistManager* GetPlaylistManager () const;
		SyncManager* GetSyncManager () const;
		SyncUnmountableManager* GetSyncUnmountableManager () const;
		CloudUploadManager* GetCloudUploadManager () const;
		ProgressManager* GetProgressManager () const;
		RadioManager* GetRadioManager () const;

		Player* GetPlayer () const;
		PreviewHandler* GetPreviewHandler () const;

		boost::optional<MediaInfo> TryURLResolve (const QUrl&) const;
	public slots:
		void rescan ();
	signals:
		void cloudStoragePluginsChanged ();

		void artistBrowseRequested (const QString&);
	};
}
}
