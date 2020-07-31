/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "proxiesstorage.h"
#include <QSettings>
#include <QCoreApplication>
#include <util/sll/qtutil.h>
#include <util/sll/prelude.h>
#include "urllistscript.h"
#include "scriptsmanager.h"

namespace LC
{
namespace XProxy
{
	ProxiesStorage::ProxiesStorage (const ScriptsManager *manager, QObject *parent)
	: QObject { parent }
	, ScriptsMgr_ { manager }
	{
	}

	QList<Proxy> ProxiesStorage::GetKnownProxies () const
	{
		return Util::Map (Proxies_, [] (const QPair<Proxy, QList<ReqTarget>>& pair) { return pair.first; });
	}

	QList<Proxy> ProxiesStorage::FindMatching (const QString& reqHost, int reqPort, const QString& proto) const
	{
		static const std::map<QString, int> proto2port
		{
			{ "http", 80 },
			{ "https", 443 }
		};

		if (reqPort < 0 && !proto.isEmpty ())
		{
			const auto pos = proto2port.find (proto.toLower ());
			if (pos != proto2port.end ())
				reqPort = pos->second;
		}

		QList<Proxy> result;
		for (const auto& pair : Proxies_)
		{
			if (result.contains (pair.first))
				continue;

			if (std::any_of (pair.second.begin (), pair.second.end (),
					[&reqHost, reqPort, &proto] (const ReqTarget& target)
					{
						if (target.Port_ && reqPort > 0 && target.Port_ != reqPort)
							return false;

						if (!target.Protocols_.isEmpty () && !target.Protocols_.contains (proto))
							return false;

						if (!target.Host_.Matches (reqHost))
							return false;

						return true;
					}))
				result << pair.first;
		}
		for (const auto& pair : Util::Stlize (Scripts_))
		{
			if (result.contains (pair.first))
				continue;

			if (std::any_of (pair.second.begin (), pair.second.end (),
					[&reqHost, reqPort, &proto] (UrlListScript *script)
						{ return script->Accepts (reqHost, reqPort, proto); }))
				result << pair.first;
		}
		return result;
	}

	void ProxiesStorage::AddProxy (const Proxy& proxy)
	{
		DoOnProxiesList (proxy,
				{
					[this, &proxy] { Proxies_.append ({ proxy, {} }); },
					[] (auto) {}
				});
	}

	void ProxiesStorage::UpdateProxy (const Proxy& oldProxy, const Proxy& newProxy)
	{
		if (oldProxy == newProxy)
			return;

		const auto& existingNewTargets = GetTargets (newProxy);
		EraseFromProxiesList (newProxy);
		DoOnProxiesList (oldProxy,
				{
					[] {},
					[&existingNewTargets, &newProxy] (auto it)
					{
						it->first = newProxy;
						it->second += existingNewTargets;
					}
				});

		const auto& oldScripts = Scripts_.take (oldProxy);
		Scripts_ [newProxy] += oldScripts;
	}

	void ProxiesStorage::RemoveProxy (const Proxy& proxy)
	{
		EraseFromProxiesList (proxy);
		Scripts_.remove (proxy);
	}

	QList<ReqTarget> ProxiesStorage::GetTargets (const Proxy& proxy) const
	{
		return DoOnProxiesList<QList<ReqTarget>> (proxy,
				{
					[] { return QList<ReqTarget> {}; },
					[] (auto it) { return it->second; }
				});
	}

	void ProxiesStorage::SetTargets (const Proxy& proxy, const QList<ReqTarget>& targets)
	{
		DoOnProxiesList (proxy,
				{
					[this, &proxy, &targets] { Proxies_.append ({ proxy, targets }); },
					[&targets] (auto it) { it->second = targets; }
				});
	}

	QList<UrlListScript*> ProxiesStorage::GetScripts (const Proxy& proxy) const
	{
		return Scripts_ [proxy];
	}

	void ProxiesStorage::SetScripts (const Proxy& proxy, const QList<UrlListScript*>& lists)
	{
		Scripts_ [proxy] = lists;
		for (const auto script : lists)
			script->SetEnabled (true);
	}

	void ProxiesStorage::Swap (int row1, int row2)
	{
		using std::swap;
		swap (Proxies_ [row1], Proxies_ [row2]);
	}

	void ProxiesStorage::LoadSettings ()
	{
		Proxies_.clear ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_XProxy");
		settings.beginGroup ("SavedProxies");

		for (const auto& entry : settings.value ("Entries").value<QList<Entry_t>> ())
			DoOnProxiesList (entry.second,
					{
						[this, &entry] { Proxies_.append ({ entry.second, { entry.first } }); },
						[&entry] (auto it) { it->second += entry.first; }
					});

		for (const auto& entry : settings.value ("Scripts").value<QList<ScriptEntry_t>> ())
			if (const auto script = ScriptsMgr_->GetScript (entry.first))
			{
				Scripts_ [entry.second] << script;
				script->SetEnabled (true);
			}
			else
				qWarning () << Q_FUNC_INFO
						<< "can't restore"
						<< entry.first;

		settings.endGroup ();
	}

	void ProxiesStorage::SaveSettings () const
	{
		QList<Entry_t> entries;
		for (const auto& pair : Proxies_)
			for (const auto& target : pair.second)
				entries.append ({ target, pair.first });

		QList<ScriptEntry_t> scripts;
		for (const auto pair : Util::Stlize (Scripts_))
			for (const auto script : pair.second)
				scripts.append ({ script->GetListId (), pair.first });

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_XProxy");
		settings.beginGroup ("SavedProxies");
		settings.setValue ("Entries", QVariant::fromValue (entries));
		settings.setValue ("Scripts", QVariant::fromValue (scripts));
		settings.endGroup ();
	}

	void ProxiesStorage::EraseFromProxiesList (const Proxy& proxy)
	{
		DoOnProxiesList (proxy,
				{
					[] {},
					[this] (auto it) { Proxies_.erase (it); }
				});
	}

	namespace
	{
		template<typename R, typename ProxyType>
		R DoOnProxiesListImpl (ProxyType&& proxies,
				const Proxy& proxy,
				const Util::EitherCont<R (), R (decltype (proxies.begin ()))>& cont)
		{
			const auto pos = std::find_if (proxies.begin (), proxies.end (),
					[&proxy] (const auto& pair) { return pair.first == proxy; });
			if (pos == proxies.end ())
				return cont.Left ();
			else
				return cont.Right (pos);
		}
	}

	template<typename R>
	R ProxiesStorage::DoOnProxiesList (const Proxy& proxy,
			const Util::EitherCont<R (), R (decltype (Proxies_.begin ()))>& cont)
	{
		return DoOnProxiesListImpl<R> (Proxies_, proxy, cont);
	}

	template<typename R>
	R ProxiesStorage::DoOnProxiesList (const Proxy& proxy,
			const Util::EitherCont<R (), R (decltype (Proxies_.constBegin ()))>& cont) const
	{
		return DoOnProxiesListImpl<R> (Proxies_, proxy, cont);
	}
}
}
