/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "rss10parser.h"
#include <QtDebug>
#include <util/sll/domchildrenrange.h>
#include <util/sll/prelude.h>

namespace LC
{
namespace Aggregator
{
	RSS10Parser& RSS10Parser::Instance ()
	{
		static RSS10Parser inst;
		return inst;
	}
	
	bool RSS10Parser::CouldParse (const QDomDocument& doc) const
	{
		QDomElement root = doc.documentElement ();
		return root.tagName () == "RDF";
	}
	
	channels_container_t RSS10Parser::Parse (const QDomDocument& doc, const IDType_t& feedId) const
	{
		channels_container_t result;
	
		QMap<QString, Channel_ptr> item2Channel;
		QDomElement root = doc.documentElement ();
		for (const auto& channelDescr : Util::DomChildren (root, "channel"))
		{
			auto channel = std::make_shared<Channel> (Channel::CreateForFeed (feedId));
			channel->Title_ = channelDescr.firstChildElement ("title").text ().trimmed ();
			channel->Link_ = channelDescr.firstChildElement ("link").text ();
			channel->Description_ = channelDescr.firstChildElement ("description").text ();
			channel->PixmapURL_ = channelDescr.firstChildElement ("image").firstChildElement ("url").text ();
			channel->LastBuild_ = GetDCDateTime (channelDescr);
	
			QDomElement itemsRoot = channelDescr.firstChildElement ("items");
			QDomNodeList seqs = itemsRoot.elementsByTagNameNS (RDF_, "Seq");
			if (seqs.isEmpty ())
				continue;
	
			QDomElement seqElem = seqs.at (0).toElement ();
			QDomNodeList lis = seqElem.elementsByTagNameNS (RDF_, "li");
			for (int i = 0; i < lis.size (); ++i)
				item2Channel [lis.at (i).toElement ().attribute ("resource")] = channel;
	
			result.push_back (channel);
		}

		for (const auto& itemDescr : Util::DomChildren (root, "item"))
		{
			QString about = itemDescr.attributeNS (RDF_, "about");
			if (!item2Channel.contains (about))
				continue;

			auto item = std::make_shared<Item> (Item::CreateForChannel (item2Channel [about]->ChannelID_));
			item->Title_ = itemDescr.firstChildElement ("title").text ();
			item->Link_ = itemDescr.firstChildElement ("link").text ();
			item->Description_ = itemDescr.firstChildElement ("description").text ();
			GetDescription (itemDescr, item->Description_);

			item->Categories_ = GetAllCategories (itemDescr);
			item->Author_ = GetAuthor (itemDescr);
			item->PubDate_ = GetDCDateTime (itemDescr);
			item->Unread_ = true;
			item->NumComments_ = GetNumComments (itemDescr);
			item->CommentsLink_ = GetCommentsRSS (itemDescr);
			item->CommentsPageLink_ = GetCommentsLink (itemDescr);
			item->Enclosures_ = GetEncEnclosures (itemDescr, item->ItemID_);
			QPair<double, double> point = GetGeoPoint (itemDescr);
			item->Latitude_ = point.first;
			item->Longitude_ = point.second;
			if (item->Guid_.isEmpty ())
				item->Guid_ = "empty";

			item2Channel [about]->Items_.push_back (item);
		}
	
		return result;
	}
}
}
