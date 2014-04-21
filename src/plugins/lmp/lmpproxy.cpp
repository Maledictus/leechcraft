/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2012  Georg Rudoy
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

#include "lmpproxy.h"
#include "core.h"
#include "util.h"
#include "localcollection.h"
#include "localfileresolver.h"
#include "previewhandler.h"
#include "util.h"

namespace LeechCraft
{
namespace LMP
{
	LMPProxy::LMPProxy ()
	{
	}

	ILocalCollection* LMPProxy::GetLocalCollection () const
	{
		return Core::Instance ().GetLocalCollection ();
	}

	ITagResolver* LMPProxy::GetTagResolver () const
	{
		return Core::Instance ().GetLocalFileResolver ();
	}

	QString LMPProxy::FindAlbumArt (const QString& nearPath, bool includeCollection) const
	{
		return FindAlbumArtPath (nearPath, !includeCollection);
	}

	QList<QFileInfo> LMPProxy::RecIterateInfo (const QString& path,
			bool followSymLinks, std::atomic<bool> *stopGuard) const
	{
		return LMP::RecIterateInfo (path, followSymLinks, stopGuard);
	}

	QMap<QString, std::function<QString (MediaInfo)>> LMPProxy::GetSubstGetters () const
	{
		return LMP::GetSubstGetters ();
	}

	QMap<QString, std::function<void (MediaInfo&, QString)>> LMPProxy::GetSubstSetters () const
	{
		return LMP::GetSubstSetters ();
	}

	QString LMPProxy::PerformSubstitutions (QString mask,
			const MediaInfo& info, SubstitutionFlags flags) const
	{
		return LMP::PerformSubstitutions (mask, info, flags);
	}

	void LMPProxy::PreviewRelease (const QString& artist,
			const QString& release, const QList<QPair<QString, int>>& tracks) const
	{
		Core::Instance ().GetPreviewHandler ()->previewAlbum (artist, release, tracks);
	}
}
}
