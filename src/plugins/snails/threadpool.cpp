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

#include "threadpool.h"
#include <util/sll/visitor.h>
#include <util/sll/delayedexecutor.h>
#include <util/sll/prelude.h>
#include <util/threads/futures.h>
#include <util/threads/monadicfuture.h>
#include "accountthread.h"
#include "accountthreadworker.h"

namespace LeechCraft
{
namespace Snails
{
	ThreadPool::ThreadPool (Account *acc, Storage *st)
	: Acc_ { acc }
	, Storage_ { st }
	{
	}

	QFuture<EitherInvokeError_t<Util::Void>> ThreadPool::TestConnectivity ()
	{
		auto thread = CreateThread ();
		ExistingThreads_ << thread;

		return thread->Schedule (TaskPriority::High, &AccountThreadWorker::TestConnectivity) *
				[this, thread] (const auto& result)
				{
					RunScheduled (thread.get ());

					Util::Visit (result.AsVariant (),
							[] (Util::Void) {},
							[this] (const auto& err)
							{
								Util::Visit (err,
										[this] (const vmime::exceptions::authentication_error& err)
										{
											qWarning () << "Snails::ThreadPool::TestConnectivity():"
													<< "initial thread authentication failed:"
													<< err.what ();
											HitLimit_ = true;
										},
										[] (const auto& e) { qWarning () << "Snails::ThreadPool::TestConnectivity():" << e.what (); });
							});

					return result;
				};
	}

	AccountThread* ThreadPool::GetThread ()
	{
		return GetNextThread ();
	}

	void ThreadPool::RunThreads ()
	{
		if (CheckingNext_)
			return;

		if (HitLimit_)
		{
			while (!Scheduled_.isEmpty ())
				Scheduled_.takeFirst () (GetNextThread ());
			return;
		}

		const auto leastBusyThread = GetNextThread ();
		if (!leastBusyThread->GetQueueSize ())
			Scheduled_.takeFirst () (leastBusyThread);

		CheckingNext_ = true;

		auto thread = CreateThread ();

		Util::Sequence (this, thread->Schedule (TaskPriority::High, &AccountThreadWorker::TestConnectivity)) >>
			[this, thread] (const auto& result)
			{
				CheckingNext_ = false;

				Util::Visit (result.AsVariant (),
						[this, thread] (Util::Void)
						{
							ExistingThreads_ << thread;
							RunScheduled (thread.get ());
						},
						[this, thread] (const auto& err)
						{
							Util::Visit (err,
									[this, thread] (const vmime::exceptions::authentication_error& err)
									{
										qWarning () << "Snails::ThreadPool::RunThreads():"
												<< "got auth error:"
												<< err.what ()
												<< "; seems like connections limit at"
												<< ExistingThreads_.size ();
										HitLimit_ = true;

										HandleThreadOverflow (thread);

										while (!Scheduled_.isEmpty ())
											Scheduled_.takeFirst () (GetNextThread ());
									},
									[] (const auto& e) { qWarning () << "Snails::ThreadPool::RunThreads():" << e.what (); });
						});
			};
	}

	AccountThread_ptr ThreadPool::CreateThread ()
	{
		const auto& threadName = "PooledThread_" + QString::number (ExistingThreads_.size ());
		const auto thread = std::make_shared<AccountThread> (false,
				threadName, Acc_, Storage_);

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, thread]
			{
				for (const auto& init : ThreadInitializers_)
					init (thread.get ());
			},
			thread.get (),
			SIGNAL (started ()),
			thread.get ()
		};

		thread->start (QThread::LowPriority);

		return thread;
	}

	void ThreadPool::RunScheduled (AccountThread *thread)
	{
		if (!Scheduled_.isEmpty ())
			Scheduled_.takeFirst () (thread);

		if (!Scheduled_.isEmpty ())
			Util::ExecuteLater ([this] { RunThreads (); }, 500);
	}

	AccountThread* ThreadPool::GetNextThread ()
	{
		const auto min = std::min_element (ExistingThreads_.begin (), ExistingThreads_.end (),
				Util::ComparingBy ([] (const auto& ptr) { return ptr->GetQueueSize (); }));
		return min->get ();
	}

	void ThreadPool::HandleThreadOverflow (AccountThread *thread)
	{
		const auto pos = std::find_if (ExistingThreads_.begin (), ExistingThreads_.end (),
				[thread] (const auto& ptr) { return ptr.get () == thread; });
		if (pos == ExistingThreads_.end ())
		{
			qWarning () << "Snails::ThreadPool::HandleThreadOverflow():"
					<< "thread"
					<< thread
					<< "is not found among existing threads";
			return;
		}

		HandleThreadOverflow (*pos);
	}

	void ThreadPool::HandleThreadOverflow (const AccountThread_ptr& thread)
	{
		thread->Schedule (TaskPriority::Low, &AccountThreadWorker::Disconnect);

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[thread] {},
			thread.get (),
			SIGNAL (finished ()),
			nullptr
		};
		thread->quit ();

		ExistingThreads_.removeOne (thread);
	}
}
}
