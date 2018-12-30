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

#include "externalresourcemanager.h"
#include <stdexcept>
#include <QtDebug>
#include <util/xpc/util.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/sys/paths.h>
#include <util/threads/futures.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"

namespace LeechCraft
{
namespace LackMan
{
	namespace
	{
		QString URLToFileName (const QUrl& url)
		{
			return QString (url.toEncoded ().toBase64 ().replace ('/', '_'));
		}
	}

	ExternalResourceManager::ExternalResourceManager (QObject *parent)
	: QObject (parent)
	, ResourcesDir_ (Util::CreateIfNotExists ("lackman/resources/"))
	{
	}

	void ExternalResourceManager::GetResourceData (const QUrl& url)
	{
		QString fileName = URLToFileName (url);

		if (ResourcesDir_.exists (fileName))
			ResourcesDir_.remove (fileName);

		if (PendingResources_.contains (url))
			return;

		QString location = ResourcesDir_.filePath (fileName);

		auto e = Util::MakeEntity (url,
				location,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		auto delegateResult = Core::Instance ().GetProxy ()->GetEntityManager ()->DelegateEntity (e);
		if (!delegateResult)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find plugin for"
					<< url;
			return;
		}

		PendingResources_ << url;

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success) { emit resourceFetched (url); },
					[=] (const IDownload::Error& error)
					{
						qWarning () << Q_FUNC_INFO
								<< "failed to download"
								<< url
								<< error.Message_;
					}
				}.Finally ([=] { PendingResources_.remove (url); });
	}

	QString ExternalResourceManager::GetResourcePath (const QUrl& url) const
	{
		return ResourcesDir_.filePath (URLToFileName (url));
	}

	void ExternalResourceManager::ClearCaches ()
	{
		Q_FOREACH (const QString& fname, ResourcesDir_.entryList ())
			ResourcesDir_.remove (fname);
	}

	void ExternalResourceManager::ClearCachedResource (const QUrl& url)
	{
		ResourcesDir_.remove (URLToFileName (url));
	}
}
}
