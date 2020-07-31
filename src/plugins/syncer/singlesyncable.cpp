/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "singlesyncable.h"
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>
#include <QTimer>
#include <QTcpSocket>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include <laretz/item.h>
#include <laretz/operation.h>
#include <laretz/packetparser.h>
#include <laretz/packetgenerator.h>
#include <interfaces/isyncable.h>

namespace LC
{
namespace Syncer
{
	SingleSyncable::SingleSyncable (const QByteArray& id, ISyncProxy *proxy, QObject *parent)
	: QObject (parent)
	, ID_ (id)
	, Proxy_ (proxy)
	, Socket_ (new QTcpSocket (this))
	{
		connect (Socket_,
				SIGNAL (connected ()),
				this,
				SLOT (handleSocketConnected ()));
		connect (Socket_,
				SIGNAL (readyRead ()),
				this,
				SLOT (handleSocketRead ()));

		QTimer::singleShot (1000,
				this,
				SLOT (startSync ()));
	}

	std::shared_ptr<QSettings> SingleSyncable::GetSettings ()
	{
		std::shared_ptr<QSettings> settings (new QSettings (QCoreApplication::organizationName (),
					QCoreApplication::applicationName () + "_Syncer_State"),
				[] (QSettings *settings) -> void
				{
					settings->endGroup ();
					delete settings;
				});
		settings->beginGroup (ID_);
		return settings;
	}

	namespace
	{
		template<typename T>
		void DoWithMaxID (const std::vector<Laretz::Operation>& ops, const T& f)
		{
			uint64_t maxSeq = 0;
			bool found = false;
			for (const auto& op : ops)
			{
				const auto& items = op.getItems ();
				const auto maxElemPos = std::max_element (items.begin (), items.end (),
						[] (const Laretz::Item& left, const Laretz::Item& right)
							{ return left.getSeq () < right.getSeq (); });
				if (maxElemPos != items.end ())
				{
					found = true;
					maxSeq = maxElemPos->getSeq ();
				}
			}

			if (found)
				f (maxSeq);
		}
	}

	void SingleSyncable::HandleList (const Laretz::ParseResult& reply)
	{
		auto deletedList = std::find_if (reply.operations.begin (), reply.operations.end (),
				[] (const Laretz::Operation& op) { return op.getType () == Laretz::OpType::Delete; });
		auto knownList = std::find_if (reply.operations.begin (), reply.operations.end (),
				[] (const Laretz::Operation& op) { return op.getType () == Laretz::OpType::List; });

		if (deletedList != reply.operations.end ())
		{
			QList<Laretz::Operation> ourOps;
			Proxy_->Merge (ourOps, { *deletedList });
			DoWithMaxID ({ *deletedList },
					[this] (uint64_t id)
					{
						GetSettings ()->setValue ("LastSyncID", static_cast<quint64> (id));
					});
		}

		if (knownList != reply.operations.end () &&
				!knownList->getItems ().empty ())
		{
			const auto& str = Laretz::PacketGenerator {}
					({ Laretz::OpType::Fetch, { knownList->getItems () } })
					({ "Login", "d34df00d" })
					({ "Password", "shitfuck" })
					();

			Socket_->write (str.c_str (), str.size ());

			State_ = State::FetchRequested;
		}
		else
			HandleFetch ({});
	}

	namespace
	{
		template<typename T>
		QList<T> ToQList (const std::vector<T>& vector)
		{
			QList<T> result;
			result.reserve (vector.size ());
			std::copy (vector.begin (), vector.end (), std::back_inserter (result));
			return result;
		}

		template<typename T>
		std::vector<T> ToStdVector (const QList<T>& list)
		{
			std::vector<T> result;
			result.reserve (list.size ());
			std::copy (list.begin (), list.end (), std::back_inserter (result));
			return result;
		}
	}

	void SingleSyncable::HandleFetch (const Laretz::ParseResult& reply)
	{
		auto ourOps = Proxy_->GetNewOps ();
		Proxy_->Merge (ourOps, ToQList (reply.operations));

		auto settings = GetSettings ();
		uint64_t maxId = settings->value ("LastSyncID").value<quint64> ();

		DoWithMaxID (reply.operations,
				[&settings, &maxId] (uint64_t id) -> void
				{
					if (id > maxId)
					{
						maxId = id;
						settings->setValue ("LastSyncID", static_cast<quint64> (id));
					}
				});

		if (ourOps.empty ())
		{
			State_ = State::Idle;
			return;
		}

		for (auto& op : ourOps)
			for (auto& item : op.getItems ())
			{
				item.setSeq (maxId);
				if (item.getParentId ().empty ())
					item.setParentId (ID_.constData ());
			}

		State_ = State::Sent;

		const auto& str = Laretz::PacketGenerator {}
				[ToStdVector (ourOps)]
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();
		Socket_->write (str.c_str (), str.size ());
	}

	void SingleSyncable::HandleRootCreated (const Laretz::ParseResult&)
	{
		const auto lastSeq = GetSettings ()->value ("LastSyncID", 0).value<uint64_t> ();

		Laretz::Item parentItem;
		parentItem.setSeq (lastSeq);
		parentItem.setParentId (ID_.constData ());

		const auto& str = Laretz::PacketGenerator {}
				({ Laretz::OpType::List, { parentItem } })
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();

		Socket_->write (str.c_str (), str.size ());

		State_ = State::ListRequested;
	}

	void SingleSyncable::HandleSendResult (const Laretz::ParseResult& reply)
	{
	}

	void SingleSyncable::CreateRoot ()
	{
		Laretz::Item parentItem;
		parentItem.setSeq (0);
		parentItem.setId (ID_.constData ());

		const auto& str = Laretz::PacketGenerator {}
				({ Laretz::OpType::Append, { parentItem } })
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();

		Socket_->write (str.c_str (), str.size ());

		State_ = State::RootCreateRequested;
	}

	void SingleSyncable::handleSocketRead ()
	{
		const auto& data = Socket_->readAll ();
		const auto& reply = Laretz::Parse ({ data.constData (), static_cast<size_t> (data.size ()) });

		qDebug () << Q_FUNC_INFO << "results:" << reply.operations.size ();
		for (auto p : reply.fields)
			qDebug () << p.first.c_str () << "->" << p.second.c_str ();

		if (reply.fields.at ("Status") != "Success")
		{
			const auto reasonPos = reply.fields.find ("ErrorCode");
			const auto reason = reasonPos != reply.fields.end () ?
					std::stoi (reasonPos->second) :
					-1;

			auto defErr = [this, reason]
			{
				qWarning () << Q_FUNC_INFO
						<< "unknown error"
						<< reason
						<< "in state"
						<< static_cast<int> (State_);
			};

			switch (State_)
			{
			case State::ListRequested:
			{
				switch (reason)
				{
				case Laretz::ErrorCode::UnknownParent:
					CreateRoot ();
					break;
				default:
					defErr ();
					break;
				}
				break;
			}
			default:
				defErr ();
				break;
			}
			return;
		}

		switch (State_)
		{
		case State::Idle:
			qWarning () << Q_FUNC_INFO
					<< "unexpected packet in Idle state";
			break;
		case State::ListRequested:
			HandleList (reply);
			break;
		case State::FetchRequested:
			HandleFetch (reply);
			break;
		case State::Sent:
			HandleSendResult (reply);
			break;
		case State::RootCreateRequested:
			HandleRootCreated (reply);
			break;
		}
	}

	void SingleSyncable::startSync ()
	{
		if (Socket_->isValid ())
			return;

		Socket_->connectToHost ("127.0.0.1", 54093);
	}

	void SingleSyncable::handleSocketConnected ()
	{
		qDebug () << Q_FUNC_INFO;

		HandleRootCreated ({});
	}
}
}
