/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Minh Ngo
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "otzerkaludownloader.h"
#include <QWebElement>
#include <QWebPage>
#include <QRegExp>
#include <QFileInfo>
#include <QDir>
#include <QUrlQuery>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/threads/futures.h>
#include <util/xpc/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/idownload.h>

namespace LC
{
namespace Otzerkalu
{
	OtzerkaluDownloader::OtzerkaluDownloader (const DownloadParams& param, const ICoreProxy_ptr& proxy, QObject *parent)
	: QObject (parent)
	, Param_ (param)
	, Proxy_ (proxy)
	{
	}

	void OtzerkaluDownloader::Begin ()
	{
		//Let's download the first URL
		Download (Param_.DownloadUrl_, Param_.RecLevel_);
	}

	QList<QUrl> OtzerkaluDownloader::CSSParser (const QString& data) const
	{
		QRegExp urlCSS { R"((?s).*?:\s*url\s*\((.*?)\).*)" };
		QList<QUrl> urlStack;
		int pos = 0;
		while ((pos = urlCSS.indexIn (data, pos)) != -1)
		{
			const QUrl url { urlCSS.cap (1) };
			if (!url.isValid ())
			{
				qWarning () << Q_FUNC_INFO
						<< "invalid URL"
						<< urlCSS.cap (1);
				continue;
			}

			urlStack.append (url);
		}
		return urlStack;
	}

	QString OtzerkaluDownloader::CSSUrlReplace (QString value, const FileData& data)
	{
		for (const auto& urlCSS : CSSParser (value))
		{
			const auto& filename = Download (urlCSS, data.RecLevel_ - 1);
			if (!filename.isEmpty ())
				value.replace (urlCSS.toString (), filename);
		}
		return value;
	}

	void OtzerkaluDownloader::HandleJobFinished (const FileData& data)
	{
		qDebug () << Q_FUNC_INFO << "Download finished";
		--UrlCount_;
		if (!data.RecLevel_ && Param_.RecLevel_)
			return;

		const QString& filename = data.Filename_;
		DownloadedFiles_ << filename;
		emit fileDownloaded (DownloadedFiles_.count ());

		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "Can't parse the file "
					<< filename
					<< ":"
					<< file.errorString ();
			return;
		}

		if (filename.section ('.', -1) == "css")
		{
			WriteData (filename, CSSUrlReplace (file.readAll (), data));
			return;
		}

		QWebPage page;
		page.mainFrame ()->setContent (file.readAll ());

		bool haveLink = false;
		for (const auto& urlElement : page.mainFrame ()->findAllElements ("*[href]") + page.mainFrame ()->findAllElements ("*[src]"))
			if (HTMLReplace (urlElement, data))
				haveLink = true;

		for (auto style : page.mainFrame ()->findAllElements ("style"))
			style.setInnerXml (CSSUrlReplace (style.toInnerXml (), data));

		if (haveLink)
			WriteData (filename, page.mainFrame ()->toHtml ());

		if (!UrlCount_)
		{
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Otzerkalu",
					tr ("Finished mirroring <em>%1</em>.")
						.arg (Param_.DownloadUrl_.toString ()),
					Priority::Info));
			emit mirroringFinished ();
		}
	}

	bool OtzerkaluDownloader::HTMLReplace (QWebElement element, const FileData& data)
	{
		bool haveHref = true;
		QUrl url = element.attribute ("href");
		if (!url.isValid ())
		{
			url = element.attribute ("src");
			haveHref = false;
		}
		if (url.isRelative ())
			url = data.Url_.resolved (url);

		if (!Param_.FromOtherSite_ && url.host () != Param_.DownloadUrl_.host ())
			return false;

		const QString& filename = Download (url, data.RecLevel_ - 1);
		if (filename.isEmpty ())
			return false;

		element.setAttribute (haveHref ? "href" : "src", filename);
		return true;
	}

	QString OtzerkaluDownloader::Download (const QUrl& url, int recLevel)
	{
		const QFileInfo fi (url.path ());
		const auto& name = fi.fileName ();
		const auto& path = Param_.DestDir_ + '/' + url.host () + fi.path ();

		//If file name's empty, rename it to 'index.html'
		const auto& file = path + '/' + (name.isEmpty () ? "index.html" : name);

		//If file's not a html file, add .html tail to the name
		const auto& filename = url.hasQuery () ?
				file + "?" + QUrlQuery { url }.toString (QUrl::FullyDecoded) + ".html" :
				file;

		//If a file's downloaded
		if (DownloadedFiles_.contains (filename))
			return QString ();

		//Create the necessary directory for the downloaded file
		QDir::root ().mkpath (path);

		auto e = Util::MakeEntity (url,
				filename,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);
		const auto result = Proxy_->GetEntityManager ()->DelegateEntity (e);
		if (!result)
		{
			qWarning () << Q_FUNC_INFO
					<< "could not download"
					<< url
					<< "to"
					<< filename;
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Otzerkalu",
					tr ("Could not download %1")
						.arg (url.toString ()),
					Priority::Critical));
			return QString ();
		}
		++UrlCount_;

		const FileData nextFileData { url, filename, recLevel };
		Util::Sequence (this, result.DownloadResult_) >>
				Util::Visitor
				{
					[this, nextFileData] (IDownload::Success) { HandleJobFinished (nextFileData); },
					[] (const IDownload::Error&) {}
				};

		return filename;
	}

	bool OtzerkaluDownloader::WriteData (const QString& filename, const QString& data)
	{
		QFile file (filename);
		if (!file.open (QIODevice::WriteOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open"
					<< filename
					<< "for writing:"
					<< file.errorString ();
			return false;
		}

		file.write (data.toUtf8 ());
		return true;
	}
}
}
