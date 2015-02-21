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

#include "brainslugz.h"
#include <util/util.h>
#include "checktab.h"
#include "progressmodelmanager.h"

namespace LeechCraft
{
namespace LMP
{
namespace BrainSlugz
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("lmp_brainslugz");

		ProgressModelManager_ = new ProgressModelManager { this };

		CoreProxy_ = proxy;

		CheckTC_ = TabClassInfo
		{
			GetUniqueID () + ".CheckTab",
			GetName (),
			GetInfo (),
			GetIcon (),
			0,
			TFOpenableByRequest | TFSingle
		};
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.LMP.BrainSlugz";
	}

	QString Plugin::GetName () const
	{
		return "LMP BrainSlugz";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Check if your collection misses some albums or EPs!");
	}

	QIcon Plugin::GetIcon () const
	{
		return {};
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return { CheckTC_ };
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.LMP.General";
		return result;
	}

	void Plugin::SetLMPProxy (ILMPProxy_ptr proxy)
	{
		LmpProxy_ = proxy;
	}

	void Plugin::TabOpenRequested (const QByteArray& tc)
	{
		if (tc == CheckTC_.TabClass_)
		{
			if (!OpenedTab_)
			{
				OpenedTab_ = new CheckTab { LmpProxy_, CoreProxy_, CheckTC_, this };
				connect (OpenedTab_,
						SIGNAL (removeTab (QWidget*)),
						this,
						SIGNAL (removeTab (QWidget*)));
			}
			emit addNewTab ("BrainSlugz", OpenedTab_);
			emit raiseTab (OpenedTab_);
		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tc;
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_lmp_brainslugz, LeechCraft::LMP::BrainSlugz::Plugin)
