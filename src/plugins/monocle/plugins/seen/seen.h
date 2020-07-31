/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/iplugin2.h>
#include <interfaces/monocle/ibackendplugin.h>
#include <interfaces/monocle/iknowfileextensions.h>
#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

namespace LC
{
namespace Monocle
{
namespace Seen
{
	class DocManager;

	class Plugin : public QObject
				 , public IInfo
				 , public IPlugin2
				 , public IBackendPlugin
				 , public IKnowFileExtensions
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IPlugin2
				LC::Monocle::IKnowFileExtensions
				LC::Monocle::IBackendPlugin)

		LC_PLUGIN_METADATA ("org.LeechCraft.Monocle.Seen")

		ddjvu_context_t *Context_;
		DocManager *DocMgr_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		QSet<QByteArray> GetPluginClasses () const;

		LoadCheckResult CanLoadDocument (const QString&);
		IDocument_ptr LoadDocument (const QString&);
		QStringList GetSupportedMimes () const;

		QList<ExtInfo> GetKnownFileExtensions () const;
	private slots:
		void checkMessageQueue ();
	};
}
}
}
