/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <iterator>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range.hpp>

namespace LC
{
namespace Util
{
namespace Views
{
	namespace detail
	{
		template<template<typename, typename> class PairType, typename FirstIt, typename SecondIt>
		using ValueType_t = PairType<typename std::iterator_traits<FirstIt>::value_type, typename std::iterator_traits<SecondIt>::value_type>;

		template<template<typename, typename> class PairType, typename FirstIt, typename SecondIt>
		class PairIterator : public std::iterator<std::forward_iterator_tag, ValueType_t<PairType, FirstIt, SecondIt>>
		{
			bool IsSentinel_;

			FirstIt First_;
			FirstIt FirstEnd_;
			SecondIt Second_;
			SecondIt SecondEnd_;
		public:
			PairIterator ()
			: IsSentinel_ { true }
			{
			}

			PairIterator (const FirstIt& first, const FirstIt& firstEnd,
					const SecondIt& second, const SecondIt& secondEnd)
			: IsSentinel_ { false }
			, First_ { first }
			, FirstEnd_ { firstEnd }
			, Second_ { second }
			, SecondEnd_ { secondEnd }
			{
			}

			bool operator== (const PairIterator& other) const
			{
				return (IsSentinel () && other.IsSentinel ()) ||
						(First_ == other.First_ && Second_ == other.Second_);
			}

			bool operator!= (const PairIterator& other) const
			{
				return !(*this == other);
			}

			bool IsSentinel () const
			{
				return IsSentinel_ || First_ == FirstEnd_ || Second_ == SecondEnd_;
			}

			PairIterator& operator++ ()
			{
				++First_;
				++Second_;
				return *this;
			}

			PairIterator operator++ (int)
			{
				auto it = *this;

				++First_;
				++Second_;

				return it;
			}

			PairType<typename std::iterator_traits<FirstIt>::value_type, typename std::iterator_traits<SecondIt>::value_type> operator* () const
			{
				return { *First_, *Second_ };
			}
		};

		template<typename I1, typename I2, template<typename, typename> class PairType>
		class ZipRange : public boost::iterator_range<PairIterator<PairType, I1, I2>>
		{
			using IteratorType_t = PairIterator<PairType, I1, I2>;
		public:
			template<typename C1, typename C2>
			ZipRange (C1&& c1, C2&& c2)
			: boost::iterator_range<IteratorType_t>
			{
				IteratorType_t { c1.begin (), c1.end (), c2.begin (), c2.end () },
				IteratorType_t {}
			}
			{
			}
		};
	}

	template<template<typename, typename> class PairType = QPair, typename C1, typename C2>
	detail::ZipRange<typename C1::const_iterator, typename C2::const_iterator, PairType> Zip (const C1& c1, const C2& c2)
	{
		return { c1, c2 };
	}
}
}
}
