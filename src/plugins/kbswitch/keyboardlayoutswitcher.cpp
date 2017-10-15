/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "keyboardlayoutswitcher.h"
#include <QtDebug>
#include <QWidget>
#include "xmlsettingsmanager.h"
#include <interfaces/ihavetabs.h>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

namespace LeechCraft
{
namespace KBSwitch
{
	KeyboardLayoutSwitcher::KeyboardLayoutSwitcher (QObject *parent)
	: QObject { parent }
	{
		XmlSettingsManager::Instance ().RegisterObject ("SwitchingPolicy",
				this, "setSwitchingPolicy");
		setSwitchingPolicy ();
	}

	bool KeyboardLayoutSwitcher::IsGlobalPolicy () const
	{
		return CurrentSwitchingPloicy_ == SwitchingPolicy::Global;
	}

	void KeyboardLayoutSwitcher::updateKBLayouts (QWidget *current, QWidget *prev)
	{
		if (CurrentSwitchingPloicy_ == SwitchingPolicy::Global)
			return;

		if (LastCurrentWidget_ == current)
			return;

#ifdef Q_OS_LINUX
		int xkbEventType, xkbError, xkbReason;
		int mjr = XkbMajorVersion, mnr = XkbMinorVersion;
		Display *display = NULL;
		display = XkbOpenDisplay (NULL,
				&xkbEventType,
				&xkbError,
				&mjr,
				&mnr,
				&xkbReason);

		if (CurrentSwitchingPloicy_ == SwitchingPolicy::Tab)
		{
			if (prev)
			{
				ITabWidget *itw = qobject_cast<ITabWidget*> (prev);
				if (!itw)
				{
					qWarning () << Q_FUNC_INFO
							<< current
							<< "is not an ITabWidget class";
				}
				else
				{
					connect (itw->ParentMultiTabs (),
							SIGNAL (removeTab (QWidget*)),
							this,
							SLOT (removeWidget (QWidget*)),
							Qt::UniqueConnection);

					XkbStateRec xkbState;
					XkbGetState (display, XkbUseCoreKbd, &xkbState);
					Widget2KBLayoutIndex_ [prev] = xkbState.group;

				}
			}
			if (current)
			{
				LastCurrentWidget_ = current;

				if (Widget2KBLayoutIndex_.contains (current))
				{
					int xkbGroup = Widget2KBLayoutIndex_ [current];
					if (!XkbLockGroup (display, XkbUseCoreKbd, xkbGroup))
						qWarning () << Q_FUNC_INFO
								<< "Request to change layout not send";
				}
			}
		}
		else if (CurrentSwitchingPloicy_ == SwitchingPolicy::Plugin)
		{
			if (prev)
			{
				ITabWidget *itw = qobject_cast<ITabWidget*> (prev);
				if (!itw)
				{
					qWarning () << Q_FUNC_INFO
							<< current
							<< "is not an ITabWidget class";
				}
				else
				{
					connect (itw->ParentMultiTabs (),
							SIGNAL (removeTab (QWidget*)),
							this,
							SLOT (handleRemoveWidget (QWidget*)),
							Qt::UniqueConnection);

					XkbStateRec xkbState;
					XkbGetState (display, XkbUseCoreKbd, &xkbState);
					TabClass2KBLayoutIndex_ [itw->GetTabClassInfo ().TabClass_] = xkbState.group;
				}
			}

			if (current)
			{
				ITabWidget *itw = qobject_cast<ITabWidget*> (current);
				if (!itw)
				{
					qWarning () << Q_FUNC_INFO
							<< current
							<< "is not an ITabWidget class";
				}
				else
				{
					LastCurrentWidget_ = current;
					if (TabClass2KBLayoutIndex_.contains (itw->GetTabClassInfo ().TabClass_))
					{
						int xkbGroup = TabClass2KBLayoutIndex_ [itw->GetTabClassInfo ().TabClass_];
						if (!XkbLockGroup (display, XkbUseCoreKbd, xkbGroup))
							qWarning () << Q_FUNC_INFO
									<< "Request to change layout not send";
					}
				}
			}
		}

		XCloseDisplay (display);
#endif
	}

	void KeyboardLayoutSwitcher::setSwitchingPolicy ()
	{
		const auto& propStr = XmlSettingsManager::Instance ().property ("SwitchingPolicy").toString ();
		if (propStr == "global")
			CurrentSwitchingPloicy_ = SwitchingPolicy::Global;
		else if (propStr == "plugin")
			CurrentSwitchingPloicy_ = SwitchingPolicy::Plugin;
		else if (propStr == "tab")
			CurrentSwitchingPloicy_ = SwitchingPolicy::Tab;
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown property value"
					<< propStr;
	}

	void KeyboardLayoutSwitcher::handleRemoveWidget (QWidget *widget)
	{
		Widget2KBLayoutIndex_.remove (widget);
	}

}
}
