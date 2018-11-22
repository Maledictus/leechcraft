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

#include "dumbstorage.h"

namespace LeechCraft
{
namespace Aggregator
{
	void DumbStorage::Prepare ()
	{
	}

	ids_t DumbStorage::GetFeedsIDs () const
	{
		return {};
	}

	Feed DumbStorage::GetFeed (const IDType_t&) const
	{
		return {};
	}

	std::optional<IDType_t> DumbStorage::FindFeed (const QString&) const
	{
		return {};
	}

	std::optional<Feed::FeedSettings> DumbStorage::GetFeedSettings (const IDType_t&) const
	{
		return {};
	}

	void DumbStorage::SetFeedSettings (const Feed::FeedSettings&)
	{
	}

	channels_shorts_t DumbStorage::GetChannels (const IDType_t&) const
	{
		return {};
	}

	Channel DumbStorage::GetChannel (const IDType_t&) const
	{
		return {};
	}

	std::optional<IDType_t> DumbStorage::FindChannel (const QString&, const QString&, const IDType_t&) const
	{
		return {};
	}

	void DumbStorage::TrimChannel (const IDType_t&, int, int)
	{
	}

	items_shorts_t DumbStorage::GetItems (const IDType_t&) const
	{
		return {};
	}

	int DumbStorage::GetUnreadItems (const IDType_t&) const
	{
		return {};
	}

	std::optional<Item> DumbStorage::GetItem (const IDType_t&) const
	{
		return {};
	}

	std::optional<IDType_t> DumbStorage::FindItem (const QString&, const QString&, const IDType_t&) const
	{
		return {};
	}

	std::optional<IDType_t> DumbStorage::FindItemByTitle (const QString&, const IDType_t&) const
	{
		return {};
	}

	std::optional<IDType_t> DumbStorage::FindItemByLink (const QString&, const IDType_t&) const
	{
		return {};
	}

	items_container_t DumbStorage::GetFullItems (const IDType_t&) const
	{
		return {};
	}

	void DumbStorage::AddFeed (const Feed&)
	{
	}

	void DumbStorage::AddChannel (const Channel&)
	{
	}

	void DumbStorage::AddItem (const Item&)
	{
	}

	void DumbStorage::UpdateChannel (const Channel&)
	{
	}

	void DumbStorage::UpdateChannel (const ChannelShort&)
	{
	}

	void DumbStorage::UpdateItem (const Item&)
	{
	}

	void DumbStorage::UpdateItem (const ItemShort&)
	{
	}

	void DumbStorage::RemoveItems (const QSet<IDType_t>&)
	{
	}

	void DumbStorage::RemoveChannel (const IDType_t&)
	{
	}

	void DumbStorage::RemoveFeed (const IDType_t&)
	{
	}

	bool DumbStorage::UpdateFeedsStorage (int, int)
	{
		return true;
	}

	bool DumbStorage::UpdateChannelsStorage (int, int)
	{
		return true;
	}

	bool DumbStorage::UpdateItemsStorage (int, int)
	{
		return true;
	}

	void DumbStorage::ToggleChannelUnread (const IDType_t&, bool)
	{
	}

	QList<ITagsManager::tag_id> DumbStorage::GetItemTags (const IDType_t&)
	{
		return {};
	}

	void DumbStorage::SetItemTags (const IDType_t&, const QList<ITagsManager::tag_id>& )
	{
	}

	QList<IDType_t> DumbStorage::GetItemsForTag (const ITagsManager::tag_id&)
	{
		return {};
	}

	IDType_t DumbStorage::GetHighestID (const PoolType&) const
	{
		return {};
	}
}
}
