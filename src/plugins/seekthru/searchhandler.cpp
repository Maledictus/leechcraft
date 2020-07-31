/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "searchhandler.h"
#include <QToolBar>
#include <QAction>
#include <QUrl>
#include <QFile>
#include <QDomDocument>
#include <QtDebug>
#include <interfaces/iwebbrowser.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/idownload.h>
#include <util/xpc/util.h>
#include <util/gui/selectablebrowser.h>
#include <util/sys/paths.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/threads/futures.h>
#include "core.h"
#include "xmlsettingsmanager.h"

Q_DECLARE_METATYPE (QToolBar*)

namespace LC::SeekThru
{
	const QString SearchHandler::OS_ = "http://a9.com/-/spec/opensearch/1.1/";

	SearchHandler::SearchHandler (const Description& d, IEntityManager *iem)
	: D_ (d)
	, IEM_ (iem)
	, Viewer_ (new Util::SelectableBrowser)
	, Toolbar_ (new QToolBar)
	{
		setObjectName ("SeekThru SearchHandler");
		Viewer_->Construct (Core::Instance ().GetWebBrowser ());

		Action_.reset (Toolbar_->addAction (tr ("Subscribe"), this, SLOT (subscribe ())));
		Action_->setProperty ("ActionIcon", "news-subscribe");
	}

	int SearchHandler::columnCount (const QModelIndex&) const
	{
		return 3;
	}

	QVariant SearchHandler::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid ())
			return QVariant ();

		int r = index.row ();
		switch (role)
		{
		case Qt::DisplayRole:
			switch (index.column ())
			{
			case 0:
				return SearchString_;
			case 1:
				if (Results_.at (r).TotalResults_ >= 0)
					return tr ("%n total result(s)", "", Results_.at (r).TotalResults_);
				else
					return tr ("Unknown number of results");
			case 2:
			{
				QString result = D_.ShortName_;
				switch (Results_.at (r).Type_)
				{
				case Result::TypeRSS:
					result += " (RSS)";
					break;
				case Result::TypeAtom:
					result += " (Atom)";
					break;
				case Result::TypeHTML:
					result += " (HTML)";
					break;
				}
				return result;
			}
			default:
				return QString ("");
			}
		case LC::RoleAdditionalInfo:
			if (Results_.at (r).Type_ == Result::TypeHTML)
			{
				Viewer_->SetNavBarVisible (XmlSettingsManager::Instance ().property ("NavBarVisible").toBool ());
				Viewer_->SetHtml (Results_.at (r).Response_, Results_.at (r).RequestURL_.toString ());
				return QVariant::fromValue<QWidget*> (Viewer_.get ());
			}
			else
				return 0;
		case LC::RoleControls:
			if (Results_.at (r).Type_ != Result::TypeHTML)
			{
				Action_->setData (r);
				return QVariant::fromValue<QToolBar*> (Toolbar_.get ());
			}
			else
				return 0;
		default:
			return QVariant ();
		}
	}

	Qt::ItemFlags SearchHandler::flags (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return {};
		else
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant SearchHandler::headerData (int, Qt::Orientation orient, int role) const
	{
		if (orient == Qt::Horizontal && role == Qt::DisplayRole)
			return QString ("");
		else
			return QVariant ();
	}

	QModelIndex SearchHandler::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return QModelIndex ();

		return createIndex (row, column);
	}

	QModelIndex SearchHandler::parent (const QModelIndex&) const
	{
		return QModelIndex ();
	}

	int SearchHandler::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : Results_.size ();
	}

	void SearchHandler::Start (const Request& r)
	{
		SearchString_ = r.String_;
		for (const auto& u : D_.URLs_)
		{
			const auto& url = u.MakeUrl (r.String_, r.Params_);

			QString fname = Util::GetTemporaryName ();
			auto e = Util::MakeEntity (url,
					fname,
					LC::Internal |
						LC::DoNotNotifyUser |
						LC::DoNotSaveInHistory |
						LC::NotPersistent |
						LC::DoNotAnnounceEntity,
					u.Type_);

			Result job;
			job.RequestURL_ = url;
			if (u.Type_ == "application/rss+xml")
				job.Type_ = Result::TypeRSS;
			else if (u.Type_ == "application/atom+xml")
				job.Type_ = Result::TypeAtom;
			else if (u.Type_.startsWith ("text/"))
				job.Type_ = Result::TypeHTML;
			else
			{
				qWarning () << Q_FUNC_INFO
						<< "unknown type"
						<< u.Type_;
				continue;
			}

			auto result = IEM_->DelegateEntity (e);
			if (!result)
			{
				emit error (tr ("Job for request<br />%1<br />wasn't delegated.")
						.arg (url.toString ()));
				continue;
			}

			Util::Sequence (this, result.DownloadResult_) >>
					Util::Visitor
					{
						[=] (const IDownload::Error&)
						{
							emit error (tr ("Search request for URL<br />%1<br />was delegated, but it failed.")
									.arg (url.toString ()));
						},
						[=] (IDownload::Success) { HandleJobFinished (job, fname); }
					};
		}
	}

	void SearchHandler::HandleJobFinished (Result result, const QString& filename)
	{
		QFile file { filename };
		if (!file.open (QIODevice::ReadOnly))
		{
			emit error (tr ("Could not open file %1.")
					.arg (filename));
			return;
		}

		result.Response_ = QString::fromUtf8 (file.readAll ());
		result.TotalResults_ = -1;

		if (!file.remove ())
			qWarning () << Q_FUNC_INFO
					<< "unable to remove temporary file"
					<< filename;

		QDomDocument doc;
		if (doc.setContent (result.Response_, true))
		{
			if (result.Type_ == Result::TypeHTML)
			{
				auto nodes = doc.elementsByTagName ("meta");
				for (int i = 0; i < nodes.size (); ++i)
				{
					auto meta = nodes.at (i).toElement ();
					if (meta.isNull ())
						continue;

					if (meta.attribute ("name") == "totalResults")
					{
						result.TotalResults_ = meta.attribute ("content").toInt ();
						break;
					}
				}
			}
			else
			{
				auto nodes = doc.elementsByTagNameNS (OS_, "totalResults");
				if (nodes.size ())
				{
					auto tr = nodes.at (0).toElement ();
					if (!tr.isNull ())
						result.TotalResults_ = tr.text ().toInt ();
				}
			}
		}

		beginInsertRows (QModelIndex (), Results_.size (), Results_.size ());
		Results_ << result;
		endInsertRows ();
	}

	void SearchHandler::subscribe ()
	{
		int r = qobject_cast<QAction*> (sender ())->data ().toInt ();

		QString mime;
		if (Results_.at (r).Type_ == Result::TypeAtom)
			mime = "application/atom+xml";
		else if (Results_.at (r).Type_ == Result::TypeRSS)
			mime = "application/rss+xml";

		auto e = Util::MakeEntity (Results_.at (r).RequestURL_,
				{},
				LC::FromUserInitiated,
				mime);
		IEM_->HandleEntity (e);
	}
}
