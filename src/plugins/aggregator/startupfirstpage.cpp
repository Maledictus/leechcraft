/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/
#include "startupfirstpage.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace Aggregator
{
	StartupFirstPage::StartupFirstPage (QWidget *parent)
	: QWizardPage (parent)
	{
		Ui_.setupUi (this);

		setTitle ("Aggregator");
		setSubTitle (tr ("Set default options"));
	}

	void StartupFirstPage::initializePage ()
	{
		connect (wizard (),
				&QWizard::accepted,
				this,
				&StartupFirstPage::handleAccepted,
				Qt::UniqueConnection);
		XmlSettingsManager::Instance ()->
				setProperty ("StartupVersion", 1);
	}

	void StartupFirstPage::handleAccepted ()
	{
		XmlSettingsManager::Instance ()->setProperty ("ShowIconInTray", Ui_.ShowIconInTray_->isChecked ());
		XmlSettingsManager::Instance ()->setProperty ("UpdateInterval", Ui_.UpdateInterval_->value ());
		XmlSettingsManager::Instance ()->setProperty ("ItemsPerChannel", Ui_.ItemsPerChannel_->value ());
		XmlSettingsManager::Instance ()->setProperty ("ItemsMaxAge", Ui_.ItemsMaxAge_->value ());
	}
}
}
