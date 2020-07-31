/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <memory>
#include <QMetaType>
#include <QtNetwork/QNetworkAccessManager>

class IShortcutProxy;
class IMWProxy;
class ITagsManager;
class IPluginsManager;
class ICoreTabWidget;
class IEntityManager;
class QTreeView;
class QModelIndex;
class QIcon;
class QMainWindow;
class QAbstractItemModel;
class QTabWidget;
class IColorThemeManager;
class IIconThemeManager;
class QAction;

namespace LC
{
	namespace Util
	{
		class BaseSettingsManager;
	}
}

class IRootWindowsManager;

/** @brief Proxy class for the communication with LeechCraft.
 *
 * Allows one to talk with LeechCraft, requesting and getting various
 * services.
 */
class Q_DECL_EXPORT ICoreProxy
{
public:
	virtual ~ICoreProxy () {}

	/** @brief Returns application-wide network access manager.
	 *
	 * If your plugin wants to work well with other internet-related
	 * ones and wants to integrate with application-wide cookie database
	 * and network cache, it should use the returned
	 * QNetworkAccessManager.
	 *
	 * @return The application-wide QNetworkAccessManager.
	 */
	virtual QNetworkAccessManager* GetNetworkAccessManager () const = 0;

	/** @brief Returns the shortcut proxy used to communicate with the
	 * shortcut manager.
	 *
	 * @return The application-wide shortcut proxy.
	 *
	 * @sa IShortcutProxy
	 */
	virtual IShortcutProxy* GetShortcutProxy () const = 0;

	/** @brief Maps the given index to the plugin's model.
	 */
	virtual QModelIndex MapToSource (const QModelIndex& index) const = 0;

	/** @brief Returns the LeechCraft's settings manager.
	 *
	 * In the returned settings manager you can use any property name
	 * you want if it starts from "PluginsStorage". To avoid name
	 * collisions from different plugins it's strongly encouraged to
	 * also use the plugin name in the property. So the property name
	 * would look like "PluginsStorage/PluginName/YourProperty".
	 *
	 * @return The Core settings manager.
	 */
	virtual LC::Util::BaseSettingsManager* GetSettingsManager () const = 0;

	/** @brief Returns the icon theme manager.
	 *
	 * @return The icon manager.
	 *
	 * @sa IIconThemeManager
	 */
	virtual IIconThemeManager* GetIconThemeManager () const = 0;

	/** @brief Returns the color theme manager.
	 *
	 * @return The color manager.
	 *
	 * @sa IColorThemeManager
	 */
	virtual IColorThemeManager* GetColorThemeManager () const = 0;

	/** @brief Returns the root windows manager.
	 *
	 * @return The root windows manager.
	 *
	 * @sa IRootWindowsManager
	 */
	virtual IRootWindowsManager* GetRootWindowsManager () const = 0;

	/** @brief Returns the application-wide tags manager.
	 *
	 * @return The application-wide tags manager.
	 *
	 * @sa ITagsManager
	 */
	virtual ITagsManager* GetTagsManager () const = 0;

	/** Returns the list of all possible search categories from the
	 * finders installed.
	 *
	 * This function merely aggregates all the search categories from all
	 * the plugins implementing the IFinder interface, calling
	 * IFinder::GetCategories().
	 *
	 * @return The search categories of all finder.
	 *
	 * @sa IFinder
	 */
	virtual QStringList GetSearchCategories () const = 0;

	/** @brief Returns the application's plugin manager.
	 *
	 * @return The application plugins manager.
	 *
	 * @sa IPluginsManager
	 */
	virtual IPluginsManager* GetPluginsManager () const = 0;

	/** @brief Returns the entity manager object.
	 *
	 * Entity manager is used to perform interoperation with other plugins
	 * by exchanging entity objects with them.
	 *
	 * @return The application-wide entity manager.
	 *
	 * @sa LC::Entity
	 * @sa IEntityManager
	 */
	virtual IEntityManager* GetEntityManager () const = 0;

	/** @brief Returns the version of LeechCraft core and base system.
	 *
	 * The returned strings reflects the runtime version of the Core.
	 *
	 * @return The version of the LeechCraft Core.
	 */
	virtual QString GetVersion () const = 0;

	/** @brief Registers the given action as having skinnable icons.
	 *
	 * Registers the given action so that it automatically gets its icon
	 * updated whenever the current iconset changes.
	 *
	 * @param[in] action The action to register.
	 */
	virtual void RegisterSkinnable (QAction *action) = 0;

	/** @brief Checks if LeechCraft is currently shutting down.
	 *
	 * This function returns whether shutdown sequence has been
	 * initiated.
	 *
	 * For example, in this case user interaction is discouraged.
	 *
	 * @return Whether LeechCraft is shutting down.
	 */
	virtual bool IsShuttingDown () = 0;
};

using ICoreProxy_ptr = std::shared_ptr<ICoreProxy>;

const ICoreProxy_ptr& GetProxyHolder ();

Q_DECLARE_METATYPE (ICoreProxy_ptr)
