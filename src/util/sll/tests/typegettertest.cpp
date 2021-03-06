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

#include "typegettertest.h"
#include <type_traits>
#include <QtTest>
#include <typegetter.h>

QTEST_MAIN (LC::Util::TypeGetterTest)

namespace LC
{
namespace Util
{
	namespace
	{
		template<typename T>
		void PrintType ()
		{
			qDebug () << Q_FUNC_INFO;
		}
	}

	void TypeGetterTest::testArgType ()
	{
		const auto f = [] (int, const double) {};
		static_assert (std::is_same_v<ArgType_t<decltype (f), 0>, int>);
		static_assert (std::is_same_v<ArgType_t<decltype (f), 1>, double>);
	}

	void TypeGetterTest::testArgTypeRef ()
	{
		const auto f = [] (int&, const double&) {};
		static_assert (std::is_same_v<ArgType_t<decltype (f), 0>, int&>);
		static_assert (std::is_same_v<ArgType_t<decltype (f), 1>, const double&>);
	}

	void TypeGetterTest::testArgTypeRvalueRef ()
	{
		const auto f = [] (int&&, const double&&) {};
		static_assert (std::is_same_v<ArgType_t<decltype (f), 0>, int&&>);
		static_assert (std::is_same_v<ArgType_t<decltype (f), 1>, const double&&>);
	}

	void TypeGetterTest::testRetType ()
	{
		const auto f = [] (int val, const double) { return val; };
		static_assert (std::is_same_v<RetType_t<decltype (f)>, int>);
	}

	void TypeGetterTest::testRetTypeVoid ()
	{
		const auto f = [] {};
		static_assert (std::is_same_v<RetType_t<decltype (f)>, void>);
	}

	void TypeGetterTest::testRetTypeRef ()
	{
		int x;
		const auto f = [&x] (int, const double) -> int& { return x; };
		static_assert (std::is_same_v<RetType_t<decltype (f)>, int&>);
	}

	void TypeGetterTest::testRetTypeConstRef ()
	{
		int x;
		const auto f = [&x] (int, const double) -> const int& { return x; };
		static_assert (std::is_same_v<RetType_t<decltype (f)>, const int&>);
	}
}
}
