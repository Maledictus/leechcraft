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

#pragma once

#include <memory>
#include <QVector>

namespace LeechCraft
{
namespace Util
{
	template<typename T>
	class ModelItemBase : public std::enable_shared_from_this<T>
	{
	protected:
		using T_wptr = std::weak_ptr<T>;
		using T_ptr = std::shared_ptr<T>;
		using T_cptr = std::shared_ptr<const T>;
		using TList_t = QVector<T_ptr>;

		T_wptr Parent_;
		TList_t Children_;

		ModelItemBase () = default;

		ModelItemBase (const T_wptr& parent)
		: Parent_ { parent }
		{
		}
	public:
		using iterator = typename TList_t::iterator;
		using const_iterator = typename TList_t::const_iterator;

		iterator begin ()
		{
			return Children_.begin ();
		}

		iterator end ()
		{
			return Children_.end ();
		}

		const_iterator begin () const
		{
			return Children_.begin ();
		}

		const_iterator end () const
		{
			return Children_.end ();
		}

		T_ptr GetChild (int row) const
		{
			return Children_.value (row);
		}

		const TList_t& GetChildren () const
		{
			return Children_;
		}

		int GetRowCount () const
		{
			return Children_.size ();
		}

		iterator EraseChild (iterator it)
		{
			return Children_.erase (it);
		}

		iterator EraseChildren (iterator begin, iterator end)
		{
			return Children_.erase (begin, end);
		}

		template<typename... Args>
		T_ptr AppendChild (Args&&... args)
		{
			const auto& newItem = std::make_shared<T> (std::forward<Args> (args)...);
			Children_ << newItem;
			return newItem;
		}

		template<typename... Args>
		T_ptr InsertChild (int pos, Args&&... args)
		{
			const auto& newItem = std::make_shared<T> (std::forward<Args> (args)...);
			Children_.insert (pos, newItem);
			return newItem;
		}

		T_ptr GetParent () const
		{
			return Parent_.lock ();
		}

		int GetRow (const T_ptr& item) const
		{
			return Children_.indexOf (item);
		}

		int GetRow (const T_cptr& item) const
		{
			const auto pos = std::find (Children_.begin (), Children_.end (), item);
			return pos == Children_.end () ?
					-1 :
					std::distance (Children_.begin (), pos);
		}

		int GetRow () const
		{
			return GetParent ()->GetRow (this->shared_from_this ());
		}
	};
}
}
