/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
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

#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QtDebug>
#include "channel.h"
#include "item.h"
#include "atom03parser.h"

namespace LeechCraft
{
namespace Aggregator
{
	Atom03Parser& Atom03Parser::Instance ()
	{
		static Atom03Parser inst;
		return inst;
	}
	
	bool Atom03Parser::CouldParse (const QDomDocument& doc) const
	{
		QDomElement root = doc.documentElement ();
		if (root.tagName () != "feed")
			return false;
		if (root.hasAttribute ("version") && root.attribute ("version") == "0.3")
			return true;
		return false;
	}
	
	channels_container_t Atom03Parser::Parse (const QDomDocument& doc,
			const IDType_t& feedId) const
	{
		channels_container_t channels;
		auto chan = std::make_shared<Channel> (Channel::CreateForFeed (feedId));
		channels.push_back (chan);
	
		QDomElement root = doc.documentElement ();
		chan->Title_ = root.firstChildElement ("title").text ().trimmed ();
		if (chan->Title_.isEmpty ())
			chan->Title_ = QObject::tr ("(No title)");
		chan->LastBuild_ = FromRFC3339 (root.firstChildElement ("updated").text ());
		chan->Link_ = GetLink (root);
		chan->Description_ = root.firstChildElement ("tagline").text ();
		chan->Language_ = "<>";
		chan->Author_ = GetAuthor (root);
	
		QDomElement entry = root.firstChildElement ("entry");
		while (!entry.isNull ())
		{
			chan->Items_.push_back (Item_ptr (ParseItem (entry, chan->ChannelID_)));
			entry = entry.nextSiblingElement ("entry");
		}
	
		return channels;
	}
	
	Item* Atom03Parser::ParseItem (const QDomElement& entry, const IDType_t& channelId) const
	{
		Item *item = new Item (channelId);
	
		item->Title_ = ParseEscapeAware (entry.firstChildElement ("title"));
		item->Link_ = GetLink (entry);
		item->Guid_ = entry.firstChildElement ("id").text ();
		item->Unread_ = true;
	
		QDomElement date = entry.firstChildElement ("modified");
		if (date.isNull ())
			date = entry.firstChildElement ("issued");
		item->PubDate_ = FromRFC3339 (date.text ());
	
		QDomElement summary = entry.firstChildElement ("content");
		if (summary.isNull ())
			summary = entry.firstChildElement ("summary");
		item->Description_ = ParseEscapeAware (summary);
		GetDescription (entry, item->Description_);
	
		item->Categories_ += GetAllCategories (entry);
		item->Author_ = GetAuthor (entry);
	
		item->NumComments_ = GetNumComments (entry);
		item->CommentsLink_ = GetCommentsRSS (entry);
		item->CommentsPageLink_ = GetCommentsLink (entry);
	
		item->Enclosures_ = GetEnclosures (entry, item->ItemID_);
		item->Enclosures_ += GetEncEnclosures (entry, item->ItemID_);

		QPair<double, double> point = GetGeoPoint (entry);
		item->Latitude_ = point.first;
		item->Longitude_ = point.second;
		item->MRSSEntries_ = GetMediaRSS (entry, item->ItemID_);
	
		return item;
	}
}
}
