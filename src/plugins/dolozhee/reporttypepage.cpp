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

#include "reporttypepage.h"
#include <memory>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include <QtDebug>
#include <util/xpc/downloadhandler.h>
#include <util/sll/functional.h>
#include "reportwizard.h"

namespace LeechCraft
{
namespace Dolozhee
{
	ReportTypePage::ReportTypePage (const ICoreProxy_ptr& proxy, QWidget *parent)
	: QWizardPage { parent }
	, Proxy_ { proxy }
	{
		Ui_.setupUi (this);
		Ui_.CatCombo_->addItem (QString ());
	}

	int ReportTypePage::nextId () const
	{
		switch (GetReportType ())
		{
		case Type::Feature:
			return ReportWizard::PageID::FeatureDetails;
		case Type::Bug:
			return ReportWizard::PageID::BugDetails;
		}

		qWarning () << Q_FUNC_INFO
				<< "invalid report type";
		return -1;
	}

	void ReportTypePage::initializePage ()
	{
		QWizardPage::initializePage ();
		if (Ui_.CatCombo_->count () > 1)
			return;

		try
		{
			const QUrl url { "https://dev.leechcraft.org/projects/leechcraft.xml?include=issue_categories" };
			new Util::DownloadHandler (url,
					Proxy_->GetEntityManager (),
					{
						[] (IDownload::Error) {},
						Util::BindMemFn (&ReportTypePage::ParseCategories, this)
					},
					this);
		}
		catch (const std::exception& e)
		{
			qDebug () << Q_FUNC_INFO
					<< e.what ();
		}
	}

	void ReportTypePage::ForceReportType (Type type)
	{
		switch (type)
		{
		case Type::Feature:
			Ui_.TypeCombo_->setCurrentIndex (1);
			break;
		case Type::Bug:
			Ui_.TypeCombo_->setCurrentIndex (0);
			break;
		default:
			return;
		}

		Ui_.TypeCombo_->setEnabled (false);
	}

	ReportTypePage::Type ReportTypePage::GetReportType () const
	{
		return Ui_.TypeCombo_->currentIndex () == 1 ? Type::Feature : Type::Bug;
	}

	int ReportTypePage::GetCategoryID () const
	{
		const int idx = Ui_.CatCombo_->currentIndex ();
		return idx > 0 ?
				Ui_.CatCombo_->itemData (idx).toInt () :
				-1;
	}

	QString ReportTypePage::GetCategoryName () const
	{
		return Ui_.CatCombo_->currentText ();
	}

	ReportTypePage::Priority ReportTypePage::GetPriority () const
	{
		return static_cast<Priority> (Ui_.PriorityBox_->currentIndex ());
	}

	void ReportTypePage::ParseCategories (const QByteArray& data)
	{
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid data"
					<< data;
			return;
		}

		auto category = doc.documentElement ()
				.firstChildElement ("issue_categories")
				.firstChildElement ("issue_category");
		while (!category.isNull ())
		{
			std::shared_ptr<void> guard (static_cast<void*> (0),
					[&category] (void*) { category = category.nextSiblingElement ("issue_category"); });
			const auto& idText = category.attribute ("id");
			bool ok = false;
			const int id = idText.toInt (&ok);
			if (!ok)
			{
				qWarning () << Q_FUNC_INFO
						<< "invalid category id"
						<< idText;
				continue;
			}

			const auto& name = category.attribute ("name");

			Ui_.CatCombo_->addItem (name, id);
		}
	}

	void ReportTypePage::handleCategoriesFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid reply"
					<< sender ();
			return;
		}

		reply->deleteLater ();

		ParseCategories (reply->readAll ());
	}
}
}
