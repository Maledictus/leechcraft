/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <functional>
#include <QObject>
#include "sllconfig.h"

namespace LC
{
namespace Util
{
	/** @brief Base class for SlotClosure.
	 */
	class UTIL_SLL_API SlotClosureBase : public QObject
	{
		Q_OBJECT
	public:
		using QObject::QObject;

		virtual ~SlotClosureBase () = default;
	public Q_SLOTS:
		/** @brief Triggers the function.
		 */
		virtual void run () = 0;
	};

	/** @brief Executes a given functor upon a signal (or a list of
	 * signals).
	 *
	 * Refer to the documentation of SlotClosureBase to check
	 * constructors and their parameters.
	 *
	 * Typical usage:
	 * \code{.cpp}
		const auto reply = networkAccessManager->get (request);        // say we want to handle a reply
		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[reply]
			{
				if (reply->error () == QNetworkReply::NoError)
					HandleData (reply->readAll ());
				reply->deleteLater ();
			},
			reply,
			SIGNAL (finished ()),
			reply
		};
	   \endcode
	 *
	 * \section LifetimeMgmt Lifetime management.
	 *
	 * The instance of this class can either be deleted after the
	 * matching signal is emitted, or when the object's parent object is
	 * deleted, or when this object is deleted explicitly by the user.
	 * The exact behavior is controlled by the \em FireDestrPolicy
	 * template policy.
	 *
	 * There are two predefined policies: DeleteLaterPolicy and
	 * NoDeletePolicy.
	 *
	 * DeleteLaterPolicy deletes the instance of this class after the
	 * signal is fired for the first time.
	 *
	 * NoDeletePolicy does not delete the object at all. In this case the
	 * object will be deleted either explicitly by the user or when its
	 * parent QObject is deleted.
	 *
	 * @tparam FireDestrPolicy Controls how the object should be
	 * destroyed in response to the watched signal.
	 */
	template<typename FireDestrPolicy>
	class SlotClosure : public SlotClosureBase
					  , public FireDestrPolicy
	{
	public:
		using FunType_t = std::function<typename FireDestrPolicy::Signature_t>;
	private:
		FunType_t Func_;
	public:
		/** @brief Constructs a SlotClosure running a given \em func with
		 * the given \em parent as a QObject.
		 *
		 * This constructor does not automatically connect to any signals.
		 * Thus, all interesting signals should be manually connected to
		 * the construct object's <code>run()</code> slot:
		 * \code{.cpp}
			const auto closure = new SlotClosure<DeleteLaterPolicy> { someFunc, parent };
			connect (object,
					SIGNAL (triggered ()),
					closure,
					SLOT (run ()));
		   \endcode
		 *
		 * @param[in] func The function to run when a connected signal is
		 * fired.
		 * @param[in] parent The parent object of this SlotClosure.
		 */
		SlotClosure (const FunType_t& func, QObject *parent)
		: SlotClosureBase { parent }
		, Func_ { func }
		{
		}

		/** @brief Constructs a SlotClosure running a given \em func with
		 * the given \em parent as a QObject on the given \em signal.
		 *
		 * @param[in] func The function to run when a matching signal is
		 * fired.
		 * @param[in] sender The sender of the signal to connect to.
		 * @param[in] signal The signal that should trigger the \em func.
		 * @param[in] parent The parent object of this SlotClosure.
		 */
		SlotClosure (const FunType_t& func,
				QObject *sender,
				const char *signal,
				QObject *parent)
		: SlotClosureBase { parent }
		, Func_ { func }
		{
			connect (sender,
					signal,
					this,
					SLOT (run ()));
		}

		/** @brief Constructs a SlotClosure running a given \em func with
		 * the given \em parent as a QObject on the given \em signalsList.
		 *
		 * @param[in] func The function to run when a matching signal is
		 * fired.
		 * @param[in] sender The sender of the signal to connect to.
		 * @param[in] signalsList The list of signals, any of which triggers
		 * the \em func.
		 * @param[in] parent The parent object of this SlotClosure.
		 */
		SlotClosure (const FunType_t& func,
				QObject *sender,
				const std::initializer_list<const char*>& signalsList,
				QObject *parent)
		: SlotClosureBase { parent }
		, Func_ { func }
		{
			for (const auto signal : signalsList)
				connect (sender,
						signal,
						this,
						SLOT (run ()));
		}

		/** @brief Triggers the function and invokes the destroy policy.
		 */
		void run () override
		{
			FireDestrPolicy::Invoke (Func_, this);
			FireDestrPolicy::Fired (this);
		}
	};

	class BasicDeletePolicy
	{
	protected:
		using Signature_t = void ();

		void Invoke (const std::function<Signature_t>& f, SlotClosureBase*)
		{
			f ();
		}

		virtual ~BasicDeletePolicy () = default;
	};

	/** @brief Deletes a SlotClosure object after its signal has fired.
	 */
	class DeleteLaterPolicy : public BasicDeletePolicy
	{
	protected:
		static void Fired (SlotClosureBase *base)
		{
			base->deleteLater ();
		}
	};

	/** @brief Does not delete a SlotClosure object.
	 */
	class NoDeletePolicy : public BasicDeletePolicy
	{
	protected:
		static void Fired (SlotClosureBase*)
		{
		}
	};

	/** @brief Delegates the SlotClosure deletion decision to the signal handler.
	 *
	 * The signal handler's return value (of enum type ChoiceDeletePolicy::Delete) is used to decide
	 * whether the SlotClosure should be deleted. This way, the signal handler may be invoked
	 * multiple times until the necessary conditions are met.
	 */
	class ChoiceDeletePolicy
	{
	public:
		/** @brief Whether the SlotClosure shall be deleted.
		 */
		enum class Delete
		{
			/** @brief Do not delete SlotClosure after this invocation.
			 */
			No,

			/** @brief Delete SlotClosure after this invocation.
			 */
			Yes
		};
	protected:
		virtual ~ChoiceDeletePolicy () {}

		using Signature_t = Delete ();

		static void Invoke (const std::function<Signature_t>& f, SlotClosureBase *base)
		{
			if (f () == Delete::Yes)
				base->deleteLater ();
		}

		static void Fired (SlotClosureBase*)
		{
		}
	};
}
}
