#include <QSettings>
#include <QDynamicPropertyChangeEvent>
#include <QtDebug>
#include <plugininterface/proxy.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace BitTorrent
		{
			using LeechCraft::Util::Proxy;

			XmlSettingsManager::XmlSettingsManager ()
			{
				LeechCraft::Util::BaseSettingsManager::Init ();
			}
			
			XmlSettingsManager* XmlSettingsManager::Instance ()
			{
				static XmlSettingsManager manager;
				return &manager;
			}
			
			QSettings* XmlSettingsManager::BeginSettings () const
			{
				QSettings *settings =
					new QSettings (Proxy::Instance ()->GetOrganizationName (),
							Proxy::Instance ()->GetApplicationName () + "_Torrent");
				return settings;
			}
			
			void XmlSettingsManager::EndSettings (QSettings*) const
			{
			}
		};
	};
};

