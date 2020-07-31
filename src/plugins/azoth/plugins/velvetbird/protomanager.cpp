/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "protomanager.h"
#include <QDir>
#include <libpurple/purple.h>
#include <libpurple/core.h>
#include <libpurple/plugin.h>
#include <util/sys/paths.h>
#include "protocol.h"
#include "account.h"
#include "buddy.h"

namespace LC
{
namespace Azoth
{
namespace VelvetBird
{
	namespace
	{
		GHashTable* GetUIInfo ()
		{
			static GHashTable *uiInfo = 0;
			if (!uiInfo)
			{
				uiInfo = g_hash_table_new (g_str_hash, g_str_equal);
				auto localUiInfo = uiInfo;
				auto add = [localUiInfo] (const char *name, const char *value)
				{
					g_hash_table_insert (localUiInfo, g_strdup (name), g_strdup (value));
				};
				add ("name", "LeechCraft VelvetBird");
				add ("version", "dummy");
				add ("website", "https://leechcraft.org");
				add ("dev_website", "https://leechcraft.org");
				add ("client_type", "pc");
			}
			return uiInfo;
		}

		class Debugger
		{
			QFile File_;
		public:
			Debugger ()
			: File_ (Util::CreateIfNotExists ("azoth/velvetbird").absoluteFilePath ("purple.log"))
			{
				if (!File_.open (QIODevice::WriteOnly))
					qWarning () << Q_FUNC_INFO
							<< "unable to open file at"
							<< File_.fileName ()
							<< File_.errorString ();
			}

			void print (PurpleDebugLevel level, const char *cat, const char *msg)
			{
				if (!File_.isOpen ())
					return;

				static const QMap<PurpleDebugLevel, QString> levels
				{
					{ PURPLE_DEBUG_ALL, "ALL" },
					{ PURPLE_DEBUG_MISC, "MISC" },
					{ PURPLE_DEBUG_INFO, "INFO" },
					{ PURPLE_DEBUG_WARNING, "WARN" },
					{ PURPLE_DEBUG_ERROR, "ERR" },
					{ PURPLE_DEBUG_FATAL, "FATAL" }
				};

				QString data = "[" + levels [level] + "] " + cat + ": " + msg + "\n";
				File_.write (data.toUtf8 ());
				File_.flush ();
			}
		};

		PurpleDebugUiOps DbgUiOps =
		{
			[] (PurpleDebugLevel level, const char *cat, const char *msg)
			{
				static Debugger dbg;
				dbg.print (level, cat, msg);
			},
			[] (PurpleDebugLevel, const char*) -> gboolean { return true; },

			NULL,
			NULL,
			NULL,
			NULL
		};

		PurpleCoreUiOps UiOps =
		{
			NULL,
			[] () { purple_debug_set_ui_ops (&DbgUiOps); },
			NULL,
			NULL,
			GetUIInfo,

			NULL,
			NULL,
			NULL
		};

		const auto PurpleReadCond = G_IO_IN | G_IO_HUP | G_IO_ERR;
		const auto PurpleWriteCond = G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL;

		struct InputClosure
		{
			PurpleInputFunction F_;
			guint Result_;
			gpointer Data_;
		};

		guint glib_input_add (gint fd, PurpleInputCondition condition,
				PurpleInputFunction function, gpointer data)
		{
			int cond = 0;

			if (condition & PURPLE_INPUT_READ)
				cond |= PurpleReadCond;
			if (condition & PURPLE_INPUT_WRITE)
				cond |= PurpleWriteCond;

			auto closure = new InputClosure { function, 0, data };

			auto channel = g_io_channel_unix_new (fd);
			auto res = g_io_add_watch_full (channel,
					G_PRIORITY_DEFAULT,
					static_cast<GIOCondition> (cond),
					[] (GIOChannel *source, GIOCondition condition, gpointer data) -> gboolean
					{
						int cond = 0;
						if (condition & PURPLE_INPUT_READ)
							cond |= PurpleReadCond;
						if (condition & PURPLE_INPUT_WRITE)
							cond |= PurpleWriteCond;

						auto closure = static_cast<InputClosure*> (data);
						closure->F_ (closure->Data_,
								g_io_channel_unix_get_fd (source),
								static_cast<PurpleInputCondition> (cond));
						return true;
					},
					closure,
					[] (gpointer data) { delete static_cast<InputClosure*> (data); });

			g_io_channel_unref(channel);
			return res;
		}

		PurpleEventLoopUiOps EvLoopOps =
		{
			g_timeout_add,
			g_source_remove,
			glib_input_add,
			g_source_remove,
			NULL,
			g_timeout_add_seconds,

			NULL,
			NULL,
			NULL
		};

		PurpleIdleUiOps IdleOps =
		{
			[] () { return time_t (); },
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};

		PurpleAccountUiOps AccUiOps =
		{
			NULL,
			[] (PurpleAccount *acc, PurpleStatus *status)
			{
				if (acc->ui_data)
					static_cast<Account*> (acc->ui_data)->HandleStatus (status);
			},
			NULL,
			NULL,
			NULL,

			NULL,
			NULL,
			NULL,
			NULL
		};

		PurpleConnectionUiOps ConnUiOps =
		{
			NULL,
			[] (PurpleConnection *gc)
			{
				static_cast<Account*> (gc->account->ui_data)->UpdateStatus ();
			},
			[] (PurpleConnection *gc)
			{
				static_cast<Account*> (gc->account->ui_data)->UpdateStatus ();
			},
			NULL,
			NULL,
			NULL,
			NULL,
			[] (PurpleConnection *gc, PurpleConnectionError reason, const char *text)
			{
				static_cast<Account*> (gc->account->ui_data)->HandleDisconnect (reason, text);
			},
			NULL,
			NULL,
			NULL
		};

		PurpleBlistUiOps BListUiOps =
		{
			nullptr,
			nullptr,
			[] (PurpleBuddyList *list) { static_cast<ProtoManager*> (list->ui_data)->Show (list); },
			[] (PurpleBuddyList *list, PurpleBlistNode *node)
				{ static_cast<ProtoManager*> (list->ui_data)->Update (list, node); },
			[] (PurpleBuddyList *list, PurpleBlistNode *node)
				{ static_cast<ProtoManager*> (list->ui_data)->Remove (list, node); },
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};

		PurpleConversationUiOps ConvUiOps =
		{
			nullptr,
			nullptr,
			nullptr,
			[] (PurpleConversation *conv, const char *who, const char *message, PurpleMessageFlags flags, time_t mtime)
			{
				if (conv->ui_data)
					static_cast<Buddy*> (conv->ui_data)->HandleMessage (who, message, flags, mtime);
				else
					static_cast<Account*> (conv->account->ui_data)->
							HandleConvLessMessage (conv, who, message, flags, mtime);
			},
			[] (PurpleConversation*, const char *name, const char *alias, const char *message, PurpleMessageFlags, time_t)
			{
				qDebug () << Q_FUNC_INFO << name << alias << message;
			},
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};

		PurpleNotifyUiOps NotifyUiOps
		{
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			[] (PurpleConnection*, const char*, PurpleNotifyUserInfo*) -> void* { qDebug () << Q_FUNC_INFO; return 0; },
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};
	}

	ProtoManager::ProtoManager (ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	{
	}

	void ProtoManager::PluginsAvailable ()
	{
		purple_debug_set_enabled (true);

		const auto& dir = Util::CreateIfNotExists ("azoth/velvetbird/purple");
		purple_util_set_user_dir (dir.absolutePath ().toUtf8 ().constData ());

		purple_core_set_ui_ops (&UiOps);
		purple_eventloop_set_ui_ops (&EvLoopOps);
		purple_idle_set_ui_ops (&IdleOps);
		purple_connections_set_ui_ops (&ConnUiOps);

		purple_set_blist (purple_blist_new ());
		purple_blist_set_ui_data (this);
		purple_blist_set_ui_ops (&BListUiOps);

		purple_conversations_set_ui_ops (&ConvUiOps);
		purple_accounts_set_ui_ops (&AccUiOps);
		purple_notify_set_ui_ops (&NotifyUiOps);

		purple_imgstore_init ();
		purple_buddy_icons_init ();

		if (!purple_core_init ("leechcraft.azoth"))
		{
			qWarning () << Q_FUNC_INFO
					<< "failed initializing libpurple";
			return;
		}

		QMap<QByteArray, Protocol_ptr> id2proto;

		auto protos = purple_plugins_get_protocols ();
		while (protos)
		{
			auto item = static_cast<PurplePlugin*> (protos->data);
			protos = protos->next;

			const auto& proto = std::make_shared<Protocol> (item, Proxy_);
			const auto& purpleId = proto->GetPurpleID ();

			if (purpleId == "prpl-jabber" || purpleId == "prpl-irc")
				continue;

			Protocols_ << proto;
			id2proto [purpleId] = proto;
		}

		auto accs = purple_accounts_get_all ();
		while (accs)
		{
			auto acc = static_cast<PurpleAccount*> (accs->data);
			accs = accs->next;

			id2proto [purple_account_get_protocol_id (acc)]->PushAccount (acc);
		}

		purple_blist_load ();
		purple_savedstatus_activate (purple_savedstatus_get_startup ());
	}

	void ProtoManager::Release ()
	{
		for (auto proto : Protocols_)
			proto->Release ();
		Protocols_.clear ();

		AccUiOps.status_changed = nullptr;
		ConnUiOps.connected = nullptr;
		ConnUiOps.disconnected = nullptr;
		ConnUiOps.report_disconnect_reason = nullptr;
		BListUiOps.show = nullptr;
		BListUiOps.update = nullptr;
		BListUiOps.remove = nullptr;
		ConvUiOps.write_im = nullptr;
		ConvUiOps.write_conv = nullptr;

		purple_core_quit ();
	}

	QList<QObject*> ProtoManager::GetProtoObjs () const
	{
		QList<QObject*> result;
		for (auto proto : Protocols_)
			result << proto.get ();
		return result;
	}

	void ProtoManager::Show (PurpleBuddyList *list)
	{
		qDebug () << Q_FUNC_INFO << list;
	}

	void ProtoManager::Update (PurpleBuddyList*, PurpleBlistNode *node)
	{
		if (node->type != PURPLE_BLIST_BUDDY_NODE)
			return;

		auto buddy = reinterpret_cast<PurpleBuddy*> (node);
		auto account = static_cast<Account*> (buddy->account->ui_data);
		account->UpdateBuddy (buddy);
	}

	void ProtoManager::Remove (PurpleBuddyList*, PurpleBlistNode *node)
	{
		if (node->type != PURPLE_BLIST_BUDDY_NODE)
			return;

		auto buddy = reinterpret_cast<PurpleBuddy*> (node);
		auto account = static_cast<Account*> (buddy->account->ui_data);
		account->RemoveBuddy (buddy);
	}
}
}
}
