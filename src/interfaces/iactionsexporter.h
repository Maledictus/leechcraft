/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#ifndef INTERFACES_IACTIONSEXPORTER_H
#define INTERFACES_IACTIONSEXPORTER_H
#include <QList>
#include <QMap>
#include <QtPlugin>

class QAction;

namespace LC
{
	/** @brief Specifies where the actions should be embedded.
	 */
	enum class ActionsEmbedPlace
	{
		/** @brief The \em Tools submenu of main LeechCraft menu.
		 */
		ToolsMenu,

		/** @brief The common tabbar context menu.
		 *
		 * This menu is used to open new tabs, close opened tabs and
		 * manipulate already opened tabs. Plugins will typically embed
		 * actions that don't relate to any currently opened tab but
		 * should be invoked easily by the user.
		 */
		CommonContextMenu,

		/** @brief The quick launch area.
		 *
		 * The quick launch area resides in the SB2 panel or similar.
		 * Actions and menus embedded in such areas are always visible
		 * by the user (unless they hide it explicitly). So, this area
		 * can be used to either perform some actions in one click (even
		 * faster than ToolsMenu) or to host actions displaying state,
		 * like a network monitor.
		 *
		 * This area is similar to LCTray but with more focus on
		 * performing actions.
		 *
		 * @sa LCTray
		 */
		QuickLaunch,

		/** @brief The context menu of the LeechCraft tray icon.
		 */
		TrayMenu,

		/** @brief The tray area.
		 *
		 * Similar to QuickLaunch, but with more focus on displaying
		 * state.
		 *
		 * @sa QuickLaunch
		 */
		LCTray
	};
}

/** @brief Interface for embedding actions and menus into various
 * places.
 */
class Q_DECL_EXPORT IActionsExporter
{
public:
	virtual ~IActionsExporter () {}

	/** @brief Returns the actions to embed.
	 *
	 * Returns the list of actions that will be inserted into the given
	 * \em area.
	 *
	 * @param[in] area The area where the actions should be placed.
	 *
	 * @return The list of actions for the given area.
	 */
	virtual QList<QAction*> GetActions (LC::ActionsEmbedPlace area) const = 0;

	/** @brief Returns the actions to embed into the menu.
	 *
	 * For each string key found in the returned map, the corresponding
	 * list of QActions would be added to the submenu under that name in
	 * the main menu. That allows several different plugins to insert
	 * actions into one menu easily.
	 *
	 * @return The map of menu name -> list of its actions.
	 */
	virtual QMap<QString, QList<QAction*>> GetMenuActions () const
	{
		return {};
	}
protected:
	/** @brief Notifies about new actions for the given area.
	 *
	 * The sender of this signal remains the owner of actions, and it
	 * may delete them at any given time.
	 *
	 * @note This function is expected to be a signal.
	 *
	 * @param[out] actions The list of new actions for the given area.
	 * @param[out] area The area where these actions should be placed.
	 */
	virtual void gotActions (QList<QAction*> actions, LC::ActionsEmbedPlace area) = 0;
};

Q_DECLARE_INTERFACE (IActionsExporter, "org.Deviant.LeechCraft.IActionsExporter/1.0")

#endif
