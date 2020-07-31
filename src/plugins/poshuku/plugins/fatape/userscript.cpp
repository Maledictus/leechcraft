/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "userscript.h"
#include <algorithm>
#include <QCoreApplication>
#include <QtDebug>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTextCodec>
#include <QTextStream>
#include <QStandardPaths>
#include <util/util.h>
#include <util/sys/paths.h>
#include <util/sll/prelude.h>
#include <util/sll/slotclosure.h>
#include <interfaces/poshuku/iwebview.h>
#include "greasemonkey.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace Poshuku
{
namespace FatApe
{
	const QString MetadataStart = "// ==UserScript==";
	const QString MetadataEnd = "// ==/UserScript==";

	UserScript::UserScript (const QString& scriptPath)
	: ScriptPath_ { scriptPath }
	, MetadataRX_ { "//\\s+@(\\S*)\\s+(.*)", Qt::CaseInsensitive }
	{
		ParseMetadata ();
		if (!Metadata_.count ("include"))
			Metadata_.insert ("include", "*");

		const auto& propName = QString ("disabled/%1%2")
				.arg (qHash (Namespace ()))
				.arg (qHash (Name ()));
		Enabled_ = !XmlSettingsManager::Instance ()->Property (propName, false).toBool ();
	}

	void UserScript::ParseMetadata ()
	{
		QFile script { ScriptPath_ };

		if (!script.open (QFile::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
				<< "unable to open file"
				<< script.fileName ()
				<< "for reading:"
				<< script.errorString ();
			return;
		}

		QTextStream content { &script };
		QString line;

		content.setCodec (QTextCodec::codecForName ("UTF-8"));
		if (content.readLine () != MetadataStart)
			return;

		while ((line = content.readLine ()) != MetadataEnd && !content.atEnd ())
		{
			if (MetadataRX_.indexIn (line) == -1)
			{
				qWarning () << Q_FUNC_INFO
						<< "incorrect format of"
						<< line;
				continue;
			}

			const auto& key = MetadataRX_.cap (1).trimmed ();
			const auto& value = MetadataRX_.cap (2).trimmed ();
			Metadata_.insert (key, value);
		}
	}

	void UserScript::BuildPatternsList (QList<QRegExp>& list, bool include) const
	{
		const QString key { include ? "include" : "exclude" };
		list = Util::Map (Metadata_.values (key),
				[] (const QString& pattern) { return QRegExp { pattern, Qt::CaseInsensitive, QRegExp::Wildcard }; });
	}

	bool UserScript::MatchToPage (const QString& pageUrl) const
	{
		QList<QRegExp> include;
		QList<QRegExp> exclude;
		auto match = [pageUrl] (QRegExp& rx)
				{ return rx.indexIn (pageUrl, 0, QRegExp::CaretAtZero) != -1; };

		BuildPatternsList (include);
		BuildPatternsList (exclude, false);

		return std::any_of (include.begin (), include.end (), match) &&
				!std::any_of (exclude.begin (), exclude.end (), match);
	}

	void UserScript::Inject (IWebView *view, IProxyObject *proxy) const
	{
		if (!Enabled_)
			return;

		QFile script { ScriptPath_ };

		if (!script.open (QFile::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
				<< "unable to open file"
				<< script.fileName ()
				<< "for reading:"
				<< script.errorString ();
			return;
		}

		const auto& gmLayerId = QString { "Greasemonkey%1%2" }
				.arg (qHash (Namespace ()))
				.arg (qHash (Name ()));
		const auto& toInject = QString { "(function (){"
			"var GM_addStyle = %1.addStyle;"
			"var GM_deleteValue = %1.deleteValue;"
			"var GM_getValue = %1.getValue;"
			"var GM_listValues = %1.listValues;"
			"var GM_setValue = %1.setValue;"
			"var GM_openInTab = %1.openInTab;"
			"var GM_getResourceText = %1.getResourceText;"
			"var GM_getResourceURL = %1.getResourceURL;"
			"var GM_log = function(){console.log.apply(console, arguments)};"
			"%2})()" }
				.arg (gmLayerId)
				.arg (QString::fromUtf8 (script.readAll ()));

		view->AddJavaScriptObject (gmLayerId, new GreaseMonkey { view, proxy, toInject, *this });
	}

	QString UserScript::Name () const
	{
		return Metadata_.value ("name", QFileInfo (ScriptPath_).baseName ());
	}

	QString UserScript::Description () const
	{
		return Metadata_.value ("description");
	}

	QString UserScript::Namespace () const
	{
		return Metadata_.value ("namespace", "Default namespace");
	}

	QString UserScript::GetResourcePath (const QString& resourceName) const
	{
		const auto& resource = QStringList { Metadata_.values ("resource") }
				.filter (QRegExp { QString ("%1\\s.*").arg (resourceName) })
				.value (0)
				.mid (resourceName.length ())
				.trimmed ();
		const QUrl resourceUrl { resource };
		const auto& resourceFile = QFileInfo { resourceUrl.path () }.fileName ();
		if (resourceFile.isEmpty ())
			return {};

		return QFileInfo
		{
			Util::CreateIfNotExists ("data/poshuku/fatape/scripts/resources"),
			QString { "%1%2_%3" }
					.arg (qHash (Namespace ()))
					.arg (qHash (Name ()))
					.arg (resourceFile)
		}.absoluteFilePath ();
	}

	QString UserScript::Path () const
	{
		return ScriptPath_;
	}

	bool UserScript::IsEnabled () const
	{
		return Enabled_;
	}

	void UserScript::SetEnabled (bool value)
	{
		const auto& propName = QString { "disabled/%1%2" }
				.arg (qHash (Namespace ()))
				.arg (qHash (Name ()))
				.toLatin1 ();
		XmlSettingsManager::Instance ()->setProperty (propName, !value);
		Enabled_ = value;
	}

	void UserScript::Delete ()
	{
		QSettings settings { QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_Poshuku_FatApe" };

		const auto& nsHash = qHash (Namespace ());
		const auto& name = Name ();

		settings.remove (QString { "storage/%1/%2" }
				.arg (nsHash)
				.arg (name));
		settings.remove (QString { "resources/%1/%2" }
				.arg (nsHash)
				.arg (name));
		settings.remove (QString { "disabled/%1%2" }
				.arg (nsHash)
				.arg (name));

		for (const auto& resource : Metadata_.values ("resource"))
			QFile::remove (GetResourcePath (resource.mid (0, resource.indexOf (" "))));
		QFile::remove (ScriptPath_);
	}

	QStringList UserScript::Include () const
	{
		return Metadata_.values ("include");
	}

	QStringList UserScript::Exclude () const
	{
		return Metadata_.values ("exclude");
	}

	bool UserScript::Install ()
	{
		const auto& temp = QStandardPaths::writableLocation (QStandardPaths::TempLocation);

		if (!ScriptPath_.startsWith (temp))
			return false;

		QFileInfo installPath
		{
			Util::CreateIfNotExists ("data/poshuku/fatape/scripts/"),
			QFileInfo { ScriptPath_ }.fileName ()
		};

		QFile::copy (ScriptPath_, installPath.absoluteFilePath ());
		ScriptPath_ = installPath.absoluteFilePath ();
		return true;
	}

	void UserScript::Install (QNetworkAccessManager *networkManager)
	{
		if (!Install ())
			return;

		for (const auto& resource : Metadata_.values ("resource"))
			DownloadResource (resource, networkManager);
		for (const auto& required : Metadata_.values ("require"))
			DownloadRequired (required, networkManager);
	}

	void UserScript::DownloadResource (const QString& resource,
			QNetworkAccessManager *networkManager)
	{
		const auto& resourceName = resource.mid (0, resource.indexOf (" "));
		const auto& resourceUrl = resource.mid (resource.indexOf (" ") + 1);
		const QNetworkRequest resourceRequest { QUrl { resourceUrl } };
		const auto reply = networkManager->get (resourceRequest);

		const auto& propName = QString { "resources/%1/%2/%3" }
				.arg (qHash (Namespace ()))
				.arg (Name ())
				.arg (resourceName);
		const auto& path = GetResourcePath (resourceName);
		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[reply, propName, path]
			{
				reply->deleteLater ();

				QFile resource { path };
				if (!resource.open (QFile::WriteOnly))
				{
					qWarning () << Q_FUNC_INFO
							<< "unable to save resource"
							<< path
							<< "from"
							<< reply->url ().toString ();
					return;
				}
				resource.write (reply->readAll ());

				XmlSettingsManager::Instance ()->setProperty (propName.toLatin1 (),
						reply->header (QNetworkRequest::ContentTypeHeader));
			},
			reply,
			SIGNAL (finished ()),
			reply
		};
	}

	void UserScript::DownloadRequired (const QString&,
			QNetworkAccessManager*)
	{
		//TODO
	}
}
}
}

