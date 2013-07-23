/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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

#include "woodpecker.h"

#include <QIcon>
#include <interfaces/entitytesthandleresult.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/util.h>
#include "core.h"
#include "twitterpage.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Woodpecker
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("woodpecker");
		
		XmlSettingsDialog_.reset (new Util::XmlSettingsDialog ());
		XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
											"woodpeckersettings.xml");
		
		Core::Instance ().SetProxy (proxy);
		
		HomeTC_ = {
			GetUniqueID () + "_home",
			tr ("Twitter Home"),
			tr ("Own timeline"),
			GetIcon (),
			2,
			TFOpenableByRequest
		};
		
		UserTC_ = {
			GetUniqueID () + "_user",
			tr ("Twitter user timeline"),
			tr ("User's timeline"),
			GetIcon (),
			2,
			TFEmpty
		};
		
		SearchTC_ = {
			GetUniqueID () + "_search",
			tr ("Twitter search timeline"),
			tr ("Twitter search result timeline"),
			GetIcon (),
			2,
			TFEmpty
		};
		
		FavoriteTC_ = {
			GetUniqueID () + "_favorites",
			tr ("Favorite"),
			tr ("Twitter favorite statuses timeline"),
			GetIcon (),
			2,
			TFEmpty
		};
	
		TabClasses_.append ({ HomeTC_,
							[this] (const TabClassInfo& tc)
								{MakeTab (new TwitterPage (tc, this), tc); }});
		TabClasses_.append ({ UserTC_, nullptr });
		TabClasses_.append ({ SearchTC_, nullptr });
		TabClasses_.append ({ FavoriteTC_, nullptr });
	}
	
	void Plugin::AddTab (const TabClassInfo& tc, const QString& name,
						 const FeedMode mode, const KQOAuthParameters& params)
	{
		if (name.isEmpty ())
			return;
		
		auto newtab = new TwitterPage (tc, this, mode, params);
		
		emit addNewTab (name, newtab);
		emit raiseTab (newtab);
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Woodpecker";
	}

	QString Plugin::GetName () const
	{
		return "Woodpecker";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Simple twitter client");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/woodpecker/resources/images/woodpecker.svg");
		return icon;
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		TabClasses_t result;
		for (const auto& item : TabClasses_)
			result << item.first;
		return result;
	}

	void Plugin::TabOpenRequested (const QByteArray& tc)
	{
		const auto pos = std::find_if (TabClasses_.begin (), TabClasses_.end (),
				[&tc] (decltype (TabClasses_.at (0)) pair)
					{ return pair.first.TabClass_ == tc; });
		
		if (pos == TabClasses_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tc;
			return;
		}
		
		pos->second (pos->first);
	}

	std::shared_ptr<Util::XmlSettingsDialog> Plugin::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	void Plugin::MakeTab (QWidget *tab, const TabClassInfo& tc)
	{
		connect (tab,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		emit addNewTab (tc.VisibleName_, tab);
		emit changeTabIcon (tab, tc.Icon_);
		emit raiseTab (tab);
	}
	
	void Plugin::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& recInfo : infos)
		{
			QDataStream stream (recInfo.Data_);
			char *buf;
			stream >> buf;
			
			const QString type (buf);
			
			if (type.startsWith ("org.LeechCraft.Woodpecker_home"))
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);

				TabOpenRequested (GetUniqueID () + "_home");
			}
			else if (type.startsWith ("org.LeechCraft.Woodpecker_user"))
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);
				
				KQOAuthParameters param;
				stream >> param;

				const auto& username = param.take ("screen_name");;
				param.insert ("screen_name", username);

				AddTab (UserTC_,
						tr ("User %1").arg (username),
						FeedMode::UserTimeline, param);
			}
			else if (type.startsWith ("org.LeechCraft.Woodpecker_search"))
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);
				
				KQOAuthParameters param;
				stream >> param;
				
				const auto& search = param.take ("q").toUtf8 ().constData ();
				param.insert ("q", search);
				
				AddTab (SearchTC_,
						tr ("Search").append (search),
						FeedMode::SearchResult, param);
			}
			else if (type.startsWith ("org.LeechCraft.Woodpecker_favorites"))
			{
				for (const auto& pair : recInfo.DynProperties_)
					setProperty (pair.first, pair.second);
				
				KQOAuthParameters param;
				stream >> param;
				
				
				const auto& username = param.take ("screen_name");;
				param.insert ("screen_name", username);
				
				AddTab (FavoriteTC_,
						tr ("Favorites of @%1").arg (username),
						FeedMode::Favorites, param);	
			}
			else
				qWarning () << Q_FUNC_INFO
						<< "unknown context"
						<< recInfo.Data_;
		}	
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_woodpecker, LeechCraft::Woodpecker::Plugin);
