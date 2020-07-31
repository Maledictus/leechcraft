/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "aboutdialog.h"
#include <QDomDocument>
#include <util/sll/prelude.h>
#include <util/sll/domchildrenrange.h>
#include "util/sys/sysinfo.h"
#include "interfaces/ihavediaginfo.h"
#include "core.h"
#include "coreproxy.h"

namespace LC
{
	namespace
	{
		struct ContributorInfo
		{
			QString Name_;
			QString Nick_;
			QString JID_;
			QString Email_;
			QStringList Roles_;
			QList<int> Years_;

			QString Fmt () const
			{
				QString result = "<strong>";
				if (!Name_.isEmpty ())
					result += Name_;
				if (!Name_.isEmpty () && !Nick_.isEmpty ())
					result += " aka ";
				if (!Nick_.isEmpty ())
					result += Nick_;
				result += "</strong><br/>";

				if (!JID_.isEmpty ())
					result += QString ("JID: <a href=\"xmpp:%1\">%1</a>")
							.arg (JID_);
				if (!JID_.isEmpty () && !Email_.isEmpty ())
					result += "<br />";
				if (!Email_.isEmpty ())
					result += QString ("Email: <a href=\"mailto:%1\">%1</a>")
							.arg (Email_);

				result += "<ul>";
				for (const auto& r : Roles_)
					result += QString ("<li>%1</li>")
							.arg (r);
				result += "</ul>";

				QString yearsStr;
				if (Years_.size () > 1 && Years_.back () - Years_.front () == Years_.size () - 1)
					yearsStr = QString::fromUtf8 ("%1–%2")
							.arg (Years_.front ())
							.arg (Years_.back ());
				else
					yearsStr = Util::Map (Years_, [] (auto year) { return QString::number (year); }).join (", ");

				result += AboutDialog::tr ("Years: %1")
						.arg (yearsStr);

				return result;
			}
		};

		QStringList ParseRoles (const QDomElement& contribElem)
		{
			return Util::Map (Util::DomChildren (contribElem.firstChildElement ("roles"), "role"),
					[] (const auto& roleElem) { return roleElem.text (); });
		}

		QList<int> ParseYears (const QDomElement& contribElem)
		{
			auto yearsElem = contribElem
					.firstChildElement ("years");
			if (yearsElem.hasAttribute ("start") &&
					yearsElem.hasAttribute ("end"))
			{
				QList<int> result;
				for (auto start = yearsElem.attribute ("start").toInt (),
						end = yearsElem.attribute ("end").toInt ();
						start <= end; ++start)
					result << start;
				return result;
			}

			return Util::Map (Util::DomChildren (yearsElem, "year"),
					[] (const auto& yearElem) { return yearElem.text ().toInt (); });
		}

		QList<ContributorInfo> ParseContributors (const QDomDocument& doc, const QString& type)
		{
			QList<ContributorInfo> result;

			for (const auto& contribElem : Util::DomChildren (doc.documentElement (), "contrib"))
			{
				if (!type.isEmpty () && contribElem.attribute ("type") != type)
					continue;

				result.append ({
						contribElem.attribute ("name"),
						contribElem.attribute ("nick"),
						contribElem.attribute ("jid"),
						contribElem.attribute ("email"),
						ParseRoles (contribElem),
						ParseYears (contribElem)
					});
			}

			return result;
		}
	}

	AboutDialog::AboutDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);

		Ui_.ProgramName_->setText (QString ("LeechCraft %1")
				.arg (CoreProxy::UnsafeWithoutDeps ()->GetVersion ()));

		SetAuthors ();
		BuildDiagInfo ();
	}

	void AboutDialog::SetAuthors ()
	{
		QFile authorsFile (":/resources/data/authors.xml");
		if (!authorsFile.open (QIODevice::ReadOnly))
		{
			qCritical () << Q_FUNC_INFO
					<< "unable to open"
					<< authorsFile.fileName ()
					<< authorsFile.errorString ();
			return;
		}

		QDomDocument authorsDoc;
		if (!authorsDoc.setContent (&authorsFile))
		{
			qCritical () << Q_FUNC_INFO
					<< "unable to parse authors file";
			return;
		}

		auto fmtAuthorInfos = [&authorsDoc] (const QString& type)
		{
			return Util::Map (ParseContributors (authorsDoc, type), &ContributorInfo::Fmt).join ("<hr/>");
		};

		Ui_.Authors_->setHtml (fmtAuthorInfos ("author"));
		Ui_.Contributors_->setHtml (fmtAuthorInfos ("contrib"));
	}

	void AboutDialog::BuildDiagInfo ()
	{
		auto text = "LeechCraft " + CoreProxy::UnsafeWithoutDeps ()->GetVersion () + "\n";
		text += QString { "Built with Qt %1, running with Qt %2\n" }
				.arg (QT_VERSION_STR)
				.arg (qVersion ());

		text += "Running on: " + Util::SysInfo::GetOSName ();
		text += "\n--------------------------------\n\n";

		QStringList loadedModules;
		QStringList unPathedModules;
		const auto pm = Core::Instance ().GetPluginManager ();
		for (const auto plugin : pm->GetAllPlugins ())
		{
			const auto& path = pm->GetPluginLibraryPath (plugin);

			const auto ii = qobject_cast<IInfo*> (plugin);
			if (path.isEmpty ())
				unPathedModules << ("* " + ii->GetName ());
			else
				loadedModules << ("* " + ii->GetName () + " (" + path + ")");

			if (const auto diagInfo = qobject_cast<IHaveDiagInfo*> (plugin))
			{
				text += "Diag info for " + ii->GetName () + ":\n";
				text += diagInfo->GetDiagInfoString ();
				text += "\n--------------------------------\n\n";
			}
		}

		text += "Normal plugins:\n" + loadedModules.join ("\n") + "\n\n";
		if (!unPathedModules.isEmpty ())
			text += "Adapted plugins:\n" + unPathedModules.join ("\n") + "\n\n";

		Ui_.DiagInfo_->setPlainText (text);
	}
}
