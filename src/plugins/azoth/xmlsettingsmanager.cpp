/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "xmlsettingsmanager.h"
#include <QCoreApplication>
#include <QColor>
#include <QDataStream>

Q_DECLARE_METATYPE (QList<QColor>);

namespace LC
{
namespace Azoth
{
	XmlSettingsManager::XmlSettingsManager ()
	{
		qRegisterMetaType<QColor> ("QColor");
		qRegisterMetaTypeStreamOperators<QColor> ("QColor");

		qRegisterMetaType<QList<QColor>> ("QList<QColor>");
		qRegisterMetaTypeStreamOperators<QList<QColor>> ("QList<QColor>");

		Util::BaseSettingsManager::Init ();
	}

	XmlSettingsManager& XmlSettingsManager::Instance ()
	{
		static XmlSettingsManager xsm;
		return xsm;
	}

	QSettings* XmlSettingsManager::BeginSettings () const
	{
		QSettings *settings = new QSettings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth");
		return settings;
	}

	void XmlSettingsManager::EndSettings (QSettings*) const
	{
	}
}
}
