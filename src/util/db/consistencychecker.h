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
#include <boost/variant.hpp>
#include <QObject>
#include "dbconfig.h"

template<typename>
class QFuture;

template<typename>
class QFutureInterface;

namespace LeechCraft
{
namespace Util
{
	class UTIL_DB_API ConsistencyChecker : public QObject
										 , public std::enable_shared_from_this<ConsistencyChecker>
	{
		const QString DBPath_;
		const QString DialogContext_;

		friend class FailedImpl;

		ConsistencyChecker (const QString& dbPath, const QString& dialogContext, QObject* = nullptr);
	public:
		static std::shared_ptr<ConsistencyChecker> Create (const QString& dbPath, const QString& dialogContext);

		struct DumpFinished
		{
			qint64 OldFileSize_;
			qint64 NewFileSize_;
		};
		struct DumpError
		{
			QString Error_;
		};
		using DumpResult_t = std::variant<DumpFinished, DumpError>;

		struct Succeeded {};
		struct IFailed
		{
			virtual QFuture<DumpResult_t> DumpReinit () = 0;
		};
		using Failed = std::shared_ptr<IFailed>;

		using CheckResult_t = std::variant<Succeeded, Failed>;

		QFuture<CheckResult_t> StartCheck ();
	private:
		CheckResult_t CheckDB ();

		QFuture<DumpResult_t> DumpReinit ();
		void DumpReinitImpl (QFutureInterface<DumpResult_t>);

		void HandleDumperFinished (QFutureInterface<DumpResult_t>, const QString&);
	};
}
}
