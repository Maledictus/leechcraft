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

#include "macplatform.h"
#include <QTimer>
#include <QtDebug>
#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/IOMessage.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CFNumber.h>

namespace LeechCraft
{
namespace Liznoo
{
namespace Battery
{
	MacPlatform::MacPlatform ()
	: PSEventsSource_ { IOPSNotificationCreateRunLoopSource ([] (void *refCon)
				{ static_cast<MacPlatform*> (refCon)->HandlePowerSourcesChanged (); },
			this) }
	{
		CFRunLoopAddSource (CFRunLoopGetCurrent (),
				PSEventsSource_,
				kCFRunLoopCommonModes);

		QTimer::singleShot (100, this, &MacPlatform::HandlePowerSourcesChanged);
	}

	MacPlatform::~MacPlatform ()
	{
		CFRunLoopRemoveSource (CFRunLoopGetCurrent (),
				PSEventsSource_,
				kCFRunLoopCommonModes);
		CFRelease (PSEventsSource_);
	}

	namespace
	{
		void SafeRelease (CFTypeRef ref)
		{
			if (ref)
				CFRelease (ref);
		}

		template<typename T, typename U>
		std::shared_ptr<typename std::remove_pointer<T>::type> MakeShared (const T& t, const U& u)
		{
			return { t, u };
		}

		template<typename T>
		struct Numeric2ID
		{
		};

		template<>
		struct Numeric2ID<int>
		{
			static const CFNumberType Value = kCFNumberIntType;
		};

		template<typename T>
		T GetNum (CFDictionaryRef dict, NSString *keyStr, T def)
		{
			auto numRef = static_cast<CFNumberRef> (CFDictionaryGetValue (dict, keyStr));

			if (!numRef)
				return def;

			int result = 0;
			CFNumberGetValue (numRef, Numeric2ID<T>::Value, &result);
			return result;
		}

		QString GetString (CFDictionaryRef dict, NSString *key, const QString& def)
		{
			auto strRef = static_cast<CFStringRef> (CFDictionaryGetValue (dict, key));
			if (!strRef)
				return def;

			return QString::fromLatin1 (CFStringGetCStringPtr (strRef, 0));
		}

		bool GetBool (CFDictionaryRef dict, NSString *key, bool def)
		{
			auto boolRef = static_cast<CFBooleanRef> (CFDictionaryGetValue (dict, key));
			return boolRef ? boolRef == kCFBooleanTrue : def;
		}
	}

	void MacPlatform::HandlePowerSourcesChanged ()
	{
		auto info = MakeShared (IOPSCopyPowerSourcesInfo (), SafeRelease);
		if (!info)
			return;

		auto sourcesList = MakeShared (IOPSCopyPowerSourcesList (info.get ()), SafeRelease);
		if (!sourcesList)
			return;

		auto matching = IOServiceMatching ("IOPMPowerSource");
		if (!matching)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot create matching dictionary";
			return;
		}

		auto entry = IOServiceGetMatchingService (kIOMasterPortDefault, matching);
		if (!entry)
			return;

		CFMutableDictionaryRef properties = nullptr;
		IORegistryEntryCreateCFProperties (entry, &properties, nullptr, 0);

		if (!properties)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot get properties";
			return;
		}

		/*
		const void **keys = new const void* [CFDictionaryGetCount (properties)];
		CFDictionaryGetKeysAndValues (properties, keys, nullptr);
		for (int i = 0; i < CFDictionaryGetCount (properties); ++i)
			qDebug () << CFStringGetCStringPtr (static_cast<CFStringRef> (keys [i]), 0);
		*/

		const auto defVoltage = GetNum<int> (properties, @kIOPMPSVoltageKey, 0) / 1000.;
		const auto defAmperage = GetNum<int> (properties, @kIOPMPSAmperageKey, 0) / 1000.;
		const auto defDesignCapacity = GetNum<int> (properties, @kIOPMPSDesignCapacityKey, 0) / 100.;
		const auto defMaxCapacity = GetNum<int> (properties, @kIOPMPSMaxCapacityKey, 0) / 100.;
		const auto defCapacity = GetNum<int> (properties, @kIOPMPSCurrentCapacityKey, 0) / 100.;
		const auto wattage = defVoltage * defAmperage;
		const auto temperature = GetNum<int> (properties, @kIOPMPSBatteryTemperatureKey, 0) / 10.;
		const auto cyclesCount = GetNum<int> (properties, @kIOPMPSCycleCountKey, 0);

		for (CFIndex i = 0; i < CFArrayGetCount (sourcesList.get ()); ++i)
		{
			auto dict = IOPSGetPowerSourceDescription (info.get (), CFArrayGetValueAtIndex (sourcesList.get (), i));

			const auto currentCapacity = GetNum<int> (dict, @kIOPSCurrentCapacityKey, 0);
			const auto maxCapacity = GetNum<int> (dict, @kIOPSMaxCapacityKey, 0);

			const auto thisVoltage = GetNum<int> (dict, @kIOPSVoltageKey, 0) / 1000.;
			const auto thisWattage = GetBool (dict, @kIOPSIsChargedKey, false) ? 0 : wattage;
			const auto thisCyclesCount = GetNum<int> (dict, @kIOPMPSCycleCountKey, 0);

			const BatteryInfo bi
			{
				GetString (dict, @kIOPSHardwareSerialNumberKey, QString ()),

				static_cast<char> (maxCapacity ? 100 * currentCapacity / maxCapacity : currentCapacity),

				GetNum<int> (dict, @kIOPSTimeToFullChargeKey, 0) * 60,
				GetNum<int> (dict, @kIOPSTimeToEmptyKey, 0) * 60,
				thisVoltage ? thisVoltage : defVoltage,

				static_cast<double> (defCapacity),
				static_cast<double> (defMaxCapacity),
				static_cast<double> (defDesignCapacity),
				thisWattage >= 0 ? thisWattage : -thisWattage,

				QString (),

				temperature,

				thisCyclesCount ? thisCyclesCount : cyclesCount
			};

			emit batteryInfoUpdated (bi);
		}

		CFRelease (properties);
	}
}
}
}
