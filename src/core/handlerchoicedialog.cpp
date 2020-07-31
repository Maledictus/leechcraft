/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "handlerchoicedialog.h"
#include <QRadioButton>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <util/sll/prelude.h>
#include "interfaces/iinfo.h"
#include "interfaces/idownload.h"
#include "interfaces/ientityhandler.h"
#include "core.h"

namespace LC
{
	HandlerChoiceDialog::HandlerChoiceDialog (const QString& entity, QWidget *parent)
	: QDialog (parent)
	, Buttons_ (new QButtonGroup)
	{
		Ui_.setupUi (this);
		Ui_.EntityLabel_->setText (entity);
		Ui_.DownloadersLabel_->hide ();
		Ui_.HandlersLabel_->hide ();

		connect (Buttons_.get (),
				SIGNAL (buttonReleased (int)),
				this,
				SLOT (populateLocationsBox ()));
	}

	void HandlerChoiceDialog::SetFilenameSuggestion (const QString& location)
	{
		Suggestion_ = location;
	}

	void HandlerChoiceDialog::Add (QObject *obj)
	{
		auto ii = qobject_cast<IInfo*> (obj);
		if (auto idl = qobject_cast<IDownload*> (obj))
			Add (ii, idl);
		if (auto ieh = qobject_cast<IEntityHandler*> (obj))
			Add (ii, ieh);
	}

	QRadioButton* HandlerChoiceDialog::AddCommon (const IInfo *ii, const QString& addedAs)
	{
		QString name;
		QString tooltip;
		QIcon icon;
		try
		{
			name = ii->GetName ();
			tooltip = ii->GetInfo ();
			icon = ii->GetIcon ();
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
				<< "could not query"
				<< e.what ()
				<< ii;
			return 0;
		}
		catch (...)
		{
			qWarning () << Q_FUNC_INFO
				<< "could not query"
				<< ii;
			return 0;
		}

		auto but = new QRadioButton (name, this);
		but->setToolTip (tooltip);
		but->setIconSize (QSize (32, 32));
		but->setIcon (icon);
		but->setProperty ("AddedAs", addedAs);
		but->setProperty ("PluginID", ii->GetUniqueID ());

		if (Buttons_->buttons ().isEmpty ())
			but->setChecked (true);

		Buttons_->addButton (but);

		Infos_ [name] = ii;

		if (!(Downloaders_.size () + Handlers_.size ()))
			populateLocationsBox ();

		return but;
	}

	bool HandlerChoiceDialog::Add (const IInfo *ii, IDownload *id)
	{
		auto button = AddCommon (ii, "IDownload");
		if (!button)
			return false;

		Ui_.DownloadersLayout_->addWidget (button);
		Ui_.DownloadersLabel_->show ();
		Downloaders_ [ii->GetName ()] = id;
		return true;
	}

	bool HandlerChoiceDialog::Add (const IInfo *ii, IEntityHandler *ih)
	{
		auto button = AddCommon (ii, "IEntityHandler");
		if (!button)
			return false;

		Ui_.HandlersLayout_->addWidget (button);
		Ui_.HandlersLabel_->show ();
		Handlers_ [ii->GetName ()] = ih;
		return true;
	}

	QObject* HandlerChoiceDialog::GetSelected () const
	{
		auto checked = Buttons_->checkedButton ();
		if (!checked)
			return 0;

		const auto& id = checked->property ("PluginID").toByteArray ();
		return Core::Instance ().GetPluginManager ()->GetPluginByID (id);
	}

	IDownload* HandlerChoiceDialog::GetDownload ()
	{
		if (!Buttons_->checkedButton () ||
				Buttons_->checkedButton ()->
					property ("AddedAs").toString () != "IDownload")
			return 0;

		return Downloaders_.value (Buttons_->
				checkedButton ()->text (), 0);
	}

	IDownload* HandlerChoiceDialog::GetFirstDownload ()
	{
		return Downloaders_.empty () ? 0 : Downloaders_.begin ().value ();
	}

	IEntityHandler* HandlerChoiceDialog::GetEntityHandler ()
	{
		return Handlers_.value (Buttons_->
				checkedButton ()->text (), 0);
	}

	IEntityHandler* HandlerChoiceDialog::GetFirstEntityHandler ()
	{
		return Handlers_.empty () ? 0 : Handlers_.begin ().value ();
	}

	QList<IEntityHandler*> HandlerChoiceDialog::GetAllEntityHandlers ()
	{
		return Handlers_.values ();
	}

	QString HandlerChoiceDialog::GetFilename ()
	{
		const QString& name = Buttons_->checkedButton ()->
			property ("PluginID").toString ();

		QString result;
		if (Ui_.LocationsBox_->currentIndex () == 0 &&
				Ui_.LocationsBox_->currentText ().isEmpty ())
			on_BrowseButton__released ();

		result = Ui_.LocationsBox_->currentText ();
		if (result.isEmpty ())
			return QString ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.setValue ("PreviousEntitySavePath", result);

		settings.beginGroup ("SavePaths");
		QStringList pluginTexts = settings.value (name).toStringList ();
		pluginTexts.removeAll (result);
		pluginTexts.prepend (result);
		pluginTexts = pluginTexts.mid (0, 20);
		settings.setValue (name, pluginTexts);
		settings.endGroup ();

		return result;
	}

	int HandlerChoiceDialog::NumChoices () const
	{
		return Buttons_->buttons ().size ();
	}

	QStringList HandlerChoiceDialog::GetPluginSavePaths (const QString& plugin) const
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.beginGroup ("SavePaths");

		const QStringList& l = settings.value (plugin).toStringList ();
		settings.endGroup ();
		return l;
	}

	void HandlerChoiceDialog::populateLocationsBox ()
	{
		while (Ui_.LocationsBox_->count () > 1)
			Ui_.LocationsBox_->removeItem (1);

		QAbstractButton *checked = Buttons_->checkedButton ();
		if (!checked)
			return;

		if (checked->property ("AddedAs").toString () == "IEntityHandler")
		{
			Ui_.LocationsBox_->setEnabled (false);
			Ui_.BrowseButton_->setEnabled (false);
			return;
		}
		Ui_.LocationsBox_->setEnabled (true);
		Ui_.BrowseButton_->setEnabled (true);

		Ui_.LocationsBox_->insertSeparator (1);

		if (Suggestion_.size ())
			Ui_.LocationsBox_->addItem (Suggestion_);

		const QString& plugin = checked->property ("PluginID").toString ();
		const QStringList& pluginTexts = GetPluginSavePaths (plugin).mid (0, 7);

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName ());
		settings.beginGroup ("SavePaths");
		QStringList otherPlugins = settings.childKeys ();
		settings.endGroup ();

		otherPlugins.removeAll (plugin);
		auto otherTextsList = Util::Map (otherPlugins,
				[this] (const QString& plugin) { return GetPluginSavePaths (plugin); });

		for (auto& otherTexts : otherTextsList)
			for (const auto& ptext : pluginTexts)
				otherTexts.removeAll (ptext);

		QStringList otherTexts;
		while (otherTexts.size () < 16)
		{
			bool added = false;

			for (auto& otherText : otherTextsList)
				if (!otherText.isEmpty ())
				{
					otherTexts += otherText.takeFirst ();
					added = true;
				}

			if (!added)
				break;
		}

		if (!pluginTexts.isEmpty ())
		{
			Ui_.LocationsBox_->addItems (pluginTexts);
			if (!otherTexts.isEmpty ())
				Ui_.LocationsBox_->insertSeparator (pluginTexts.size () + 2);
		}
		Ui_.LocationsBox_->addItems (otherTexts);

		if (!Suggestion_.isEmpty ())
			Ui_.LocationsBox_->setCurrentIndex (1);
		else
		{
			const QString& prev = settings.value ("PreviousEntitySavePath").toString ();
			if (!prev.isEmpty () &&
					pluginTexts.contains (prev))
			{
				const int pos = Ui_.LocationsBox_->findText (prev);
				if (pos != -1)
					Ui_.LocationsBox_->setCurrentIndex (pos);
			}
			else if (!pluginTexts.isEmpty ())
				Ui_.LocationsBox_->setCurrentIndex (2);
		}
	}

	void HandlerChoiceDialog::on_BrowseButton__released ()
	{
		const QString& name = Buttons_->checkedButton ()->
			property ("PluginID").toString ();

		if (Suggestion_.isEmpty ())
			Suggestion_ = GetPluginSavePaths (name).value (0, QDir::homePath ());

		const QString& result = QFileDialog::getExistingDirectory (0,
				tr ("Select save location"),
				Suggestion_);
		if (result.isEmpty ())
			return;

		Ui_.LocationsBox_->setCurrentIndex (0);
		Ui_.LocationsBox_->setItemText (0, result);
	}
}

