/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include <QDataStream>
#include <QtDebug>
#include "feed.h"
#include "channel.h"
#include "poolsmanager.h"

namespace LC
{
namespace Aggregator
{
	Feed::Feed ()
	: FeedID_ { PoolsManager::Instance ().GetPool (PTFeed).GetID () }
	{
	}
	
	Feed::Feed (IDType_t feedId)
	: FeedID_ { feedId }
	{
	}

	Feed::Feed (IDType_t id, const QString& url, const QDateTime& lastUpdate)
	: FeedID_ { id }
	, URL_ { url }
	, LastUpdate_ { lastUpdate }
	{
	}

	bool operator< (const Feed& f1, const Feed& f2)
	{
		return f1.URL_ < f2.URL_;
	}
	
	QDataStream& operator<< (QDataStream& out, const Feed& feed)
	{
		out << feed.URL_
			<< feed.LastUpdate_
			<< static_cast<quint32> (feed.Channels_.size ());
		for (quint32 i = 0; i < feed.Channels_.size (); ++i)
			out << *feed.Channels_.at (i);
		return out;
	}
	
	QDataStream& operator>> (QDataStream& in, Feed& feed)
	{
		quint32 size = 0;
		in >> feed.URL_
			>> feed.LastUpdate_
			>> size;
		for (quint32 i = 0; i < size; ++i)
		{
			auto chan = std::make_shared<Channel> ();
			in >> *chan;
			feed.Channels_.push_back (chan);
		}
		return in;
	}
}
}
