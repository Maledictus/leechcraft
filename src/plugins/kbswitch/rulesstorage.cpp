/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "rulesstorage.h"
#include <QFile>
#include <QtDebug>
#include <QStringList>
#include <QDir>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

namespace LC
{
namespace KBSwitch
{
	namespace
	{
		QString FindX11Dir ()
		{
			static const auto dirs =
			{
				"/etc/X11",
				"/usr/share/X11",
				"/usr/local/share/X11",
				"/usr/X11R6/lib/X11",
				"/usr/X11R6/lib64/X11",
				"/usr/local/X11R6/lib/X11",
				"/usr/local/X11R6/lib64/X11",
				"/usr/lib/X11",
				"/usr/lib64/X11",
				"/usr/local/lib/X11",
				"/usr/local/lib64/X11",
				"/usr/pkg/share/X11",
				"/usr/pkg/xorg/lib/X11"
			};

			for (const auto& item : dirs)
			{
				const QString itemStr (item);
				if (QFile::exists (itemStr + "/xkb/rules"))
					return itemStr;
			}

			return {};
		}

		QString FindRulesFile (Display *display, const QString& x11dir)
		{
			XkbRF_VarDefsRec vd;
			char *path = 0;
			if (XkbRF_GetNamesProp (display, &path, &vd) && path)
			{
				const QString pathStr (path);
				free (path);
				return x11dir + "/xkb/rules/" + pathStr;
			}

			for (auto rfName : { "base", "xorg", "xfree86" })
			{
				const auto rf = QString ("/xkb/rules/") + rfName;
				const auto& path = x11dir + rf;
				if (QFile::exists (path))
					return path;
			}

			return {};
		}
	}

	RulesStorage::RulesStorage (Display *dpy, QObject *parent)
	: QObject (parent)
	, Display_ (dpy)
	, X11Dir_ (FindX11Dir ())
	{
		const auto& rf = FindRulesFile (Display_, X11Dir_);
		if (rf.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "rules file wasn't found";
			return;
		}

		char *locale = { 0 };
		const auto xkbRules = XkbRF_Load (QFile::encodeName (rf).data (),
				locale, true, true);
		if (!xkbRules)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot load rules from"
					<< rf;
			return;
		}

		for (int i = 0; i < xkbRules->layouts.num_desc; ++i)
		{
			const auto& desc = xkbRules->layouts.desc [i];
			LayName2Desc_ [desc.name] = desc.desc;
			LayDesc2Name_ [desc.desc] = desc.name;
		}

		for (int i = 0; i < xkbRules->variants.num_desc; ++i)
		{
			const auto& desc = xkbRules->variants.desc [i];

			const QString descStr (desc.desc);
			const auto& split = descStr.split (':', Qt::SkipEmptyParts);
			const auto& varName = split.value (0).trimmed ();
			const auto& hrName = split.value (1).trimmed ();

			const QString name (desc.name);
			LayName2Variants_ [varName] << name;

			VarLayHR2NameVarPair_ [hrName] = qMakePair (varName, name);
		}

		QStringList pcModelStrings;
		for (int i = 0; i < xkbRules->models.num_desc; ++i)
		{
			const auto& model = xkbRules->models.desc [i];
			KBModels_ [model.name] = model.desc;

			const auto& kbString = QString ("%1 (%2)").arg (model.desc).arg (model.name);
			if (QString (model.name).startsWith ("pc10"))
				pcModelStrings << kbString;
			else
				KBModelsStrings_ << kbString;

			KBModelString2Code_ [kbString] = model.name;
		}
		pcModelStrings.sort ();
		KBModelsStrings_.sort ();
		KBModelsStrings_ = pcModelStrings + KBModelsStrings_;

		for (int i = 0; i < xkbRules->options.num_desc; ++i)
		{
			const auto& desc = xkbRules->options.desc [i];

			QString name (desc.name);
			name = name.toLower ();

			const auto colonIdx = name.indexOf (':');
			if (colonIdx <= 0)
			{
				if (name == "compat")
					name = "numpad";

				if (name != "currencysign")
					Options_ [name] [{}] = desc.desc;

				continue;
			}

			const auto& group = name.left (colonIdx);
			const auto& option = name.mid (colonIdx + 1);

			QString descStr (desc.desc);
			descStr.replace ("&lt;", "<");
			descStr.replace ("&gt;", ">");
			Options_ [group] [option] = descStr;
		}

		XkbRF_Free (xkbRules, True);
	}

	const QHash<QString, QString>& RulesStorage::GetLayoutsD2N () const
	{
		return LayDesc2Name_;
	}

	const QHash<QString, QString>& RulesStorage::GetLayoutsN2D () const
	{
		return LayName2Desc_;
	}

	const QHash<QString, QPair<QString, QString>>& RulesStorage::GetVariantsD2Layouts () const
	{
		return VarLayHR2NameVarPair_;
	}

	QStringList RulesStorage::GetLayoutVariants (const QString& layout) const
	{
		return LayName2Variants_ [layout];
	}

	const QHash<QString, QString>& RulesStorage::GetKBModels () const
	{
		return KBModels_;
	}

	const QStringList& RulesStorage::GetKBModelsStrings () const
	{
		return KBModelsStrings_;
	}

	QString RulesStorage::GetKBModelCode (const QString& string) const
	{
		return KBModelString2Code_ [string];
	}

	const QMap<QString, QMap<QString, QString>>& RulesStorage::GetOptions () const
	{
		return Options_;
	}
}
}
