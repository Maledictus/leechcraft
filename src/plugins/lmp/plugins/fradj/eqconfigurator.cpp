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

#include "eqconfigurator.h"
#include <algorithm>
#include <iterator>
#include <QSettings>
#include "eqconfiguratordialog.h"
#include "iequalizer.h"

namespace LeechCraft
{
namespace LMP
{
namespace Fradj
{
	EqConfigurator::EqConfigurator (QObject *parent)
	: QObject { parent }
	, IEq_ { dynamic_cast<IEqualizer*> (parent) }
	, ID_ { IEq_->GetEffectId () }
	, Bands_ { IEq_->GetFixedBands () }
	{
	}

	void EqConfigurator::OpenDialog ()
	{
		const auto& gains = ReadGains ();

		EqConfiguratorDialog dia { Bands_, gains };
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& newGains = dia.GetGains ();
		if (newGains == gains)
			return;

		IEq_->SetGains (newGains);
	}

	QList<double> EqConfigurator::ReadGains () const
	{
		QList<double> gains;

		QSettings settings { QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_LMP_Fadj" };
		settings.beginGroup (ID_);
		const int count = settings.beginReadArray ("Gains");

		if (count)
			for (int i = 0; i < count; ++i)
			{
				settings.setArrayIndex (i);
				gains << settings.value ("Gain").toDouble ();
			}
		else
			std::fill_n (std::back_inserter (gains), Bands_.size (), 0);

		settings.endArray ();
		settings.endGroup ();

		return gains;
	}
}
}
}
