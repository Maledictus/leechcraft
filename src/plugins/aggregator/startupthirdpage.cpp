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

#include "startupthirdpage.h"
#include <QLineEdit>
#include <QTextCodec>
#include <QDomDocument>
#include <util/util.h>
#include <util/sll/domchildrenrange.h>
#include <util/sll/prelude.h>
#include <util/sll/overload.h>
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LeechCraft
{
namespace Aggregator
{
	StartupThirdPage::StartupThirdPage (QWidget *parent)
	: QWizardPage (parent)
	{
		ParseFeedsSets ();

		Ui_.setupUi (this);

		const auto header = Ui_.Tree_->header ();
		header->setSectionResizeMode (0, QHeaderView::ResizeToContents);
		header->setSectionResizeMode (1, QHeaderView::ResizeToContents);

		connect (Ui_.LocalizationBox_,
				Util::Overload<const QString&> (&QComboBox::currentIndexChanged),
				this,
				&StartupThirdPage::HandleCurrentIndexChanged);

		const QMap<QString, int> languages
		{
			{ "ru", 1 }
		};

		const auto& language = Util::GetLanguage ();
		Ui_.LocalizationBox_->setCurrentIndex (languages.value (language, 0));
		HandleCurrentIndexChanged ("(" + language + ")");

		setTitle ("Aggregator");
		setSubTitle (tr ("Select feeds"));
	}

	void StartupThirdPage::initializePage ()
	{
		connect (wizard (),
				&QWizard::accepted,
				this,
				&StartupThirdPage::HandleAccepted,
				Qt::UniqueConnection);
		wizard ()->setMinimumWidth (std::max (wizard ()->minimumWidth (), 800));
		wizard ()->setMinimumHeight (std::max (wizard ()->minimumHeight (), 500));

		XmlSettingsManager::Instance ()->setProperty ("StartupVersion", 3);
	}

	void StartupThirdPage::ParseFeedsSets ()
	{
		QFile file (":/resources/data/default_feeds.xml");
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open feeds resource file:"
					<< file.errorString ();
			return;
		}

		QDomDocument doc;
		QString msg;
		int line = 0;
		int col = 0;
		if (!doc.setContent (&file, &msg, &line, &col))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse feed resource file:"
					<< msg
					<< "on"
					<< line
					<< col;
			return;
		}

		for (const auto& setElem : Util::DomChildren (doc.documentElement (), "set"))
		{
			auto& set = Sets_ [setElem.attribute ("lang", "general")];

			for (const auto& feedElem : Util::DomChildren (setElem, "feed"))
				set << FeedInfo
				{
					feedElem.firstChildElement ("name").text (),
					Util::Map (Util::DomChildren (feedElem.firstChildElement ("tags"), "tag"), &QDomElement::text),
					feedElem.firstChildElement ("url").text ()
				};
		}
	}

	void StartupThirdPage::Populate (const QString& title)
	{
		for (const auto& info : Sets_.value (title))
		{
			const auto& joinedTags = info.DefaultTags_.join ("; ");
			auto item = new QTreeWidgetItem (Ui_.Tree_, { info.Name_, joinedTags, info.URL_ });
			item->setCheckState (0, Qt::Checked);

			QLineEdit *edit = new QLineEdit (Ui_.Tree_);
			edit->setFrame (false);
			edit->setText (joinedTags);
			Ui_.Tree_->setItemWidget (item, 1, edit);
		}
	}

	void StartupThirdPage::HandleAccepted ()
	{
		if (wizard ()->field ("Aggregator/StorageDirty").toBool ())
			Core::Instance ().ReinitStorage ();

		QList<SelectedFeed> feeds;
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
		{
			QTreeWidgetItem *item = Ui_.Tree_->topLevelItem (i);
			if (item->checkState (0) != Qt::Checked)
				continue;

			QString url = item->text (2);
			QString tags = static_cast<QLineEdit*> (Ui_.Tree_->itemWidget (item, 1))->text ();
			feeds << SelectedFeed { url, tags };
		}

		if (!feeds.isEmpty ())
			emit feedsSelected (feeds);
	}

	void StartupThirdPage::HandleCurrentIndexChanged (const QString& text)
	{
		Ui_.Tree_->clear ();
		if (text.endsWith (')'))
		{
			QString selected = text.mid (text.size () - 3, 2);
			Populate (selected);
		}
		Populate ("general");
	}

	void StartupThirdPage::on_SelectAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Checked);
	}

	void StartupThirdPage::on_DeselectAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Unchecked);
	}
}
}
