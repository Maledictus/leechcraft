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

#ifndef PLUGINS_AZOTH_INTERFACES_IMUCJOINWIDGET_H
#define PLUGINS_AZOTH_INTERFACES_IMUCJOINWIDGET_H
#include <QVariant>

namespace LC
{
namespace Azoth
{
	/** @brief This interface defines methods that should be implemented
	 * in widgets which are used to join MUC rooms.
	 *
	 * Please note that the join widget is global for the whole protocol
	 * thus account objects are used to distinguish between different
	 * accounts when joining.
	 */
	class IMUCJoinWidget
	{
	public:
		virtual ~IMUCJoinWidget () {}

		/** @brief Called when user selects a different account of this
		 * protocol.
		 *
		 * In this function the join widget may change the account-
		 * dependent options according to the selected account, like
		 * default join nickname and such.
		 *
		 * This function is only called if the account belonging to the
		 * join widget's protocol is selected. If the user selects an
		 * account from another protocol, this function won't be called.
		 *
		 * @param[in] account The account object of the protocol that
		 * returned this widget.
		 */
		virtual void AccountSelected (QObject *account) = 0;

		/** @brief Called when user decides to join a chatroom from the
		 * given account.
		 *
		 * @param[in] account The account object through which the user
		 * wishes to join.
		 */
		virtual void Join (QObject *account) = 0;

		/** @brief Called when user decides to not join any chatrooms.
		 */
		virtual void Cancel () = 0;

		/** @brief Returns the map with current join parameters.
		 *
		 * The returned map could be saved by Azoth core to be used
		 * later to restore the join parameters, for example, if they
		 * were saved to history, and the user decides to pick a recent
		 * join from the history.
		 *
		 * The returned map should have two mandatory fields. First is
		 * HumanReadableName, its value should be a human-readable
		 * QString, it is used to visually represent this item to the
		 * user. Second field is AccountID, the corresponding value
		 * should be a QByteArray with the ID of the selected account
		 * (IAccount::GetAccountID()).
		 *
		 * Of course, all the data in the map should be serializable,
		 * since it would be stored in QSettings.
		 *
		 * You are free to use any other fields.
		 *
		 * @return Join parameters map.
		 *
		 * @sa SetIdentifyingData()
		 */
		virtual QVariantMap GetIdentifyingData () const = 0;

		/** @brief Sets the previously saved join parameters.
		 *
		 * This function is called when the user selects a previously
		 * saved join. See the documentation for GetIdentifyingData()
		 * for more information regarding the map's contents.
		 *
		 * @param[in] data Join parameters map.
		 *
		 * @sa GetIdentifyingData()
		 */
		virtual void SetIdentifyingData (const QVariantMap& data) = 0;

		/** @brief Notifies about validity of the input.
		 *
		 * This signal should be emitted whenever the validity of the
		 * input data is changed.
		 *
		 * @note This function is expected to be a signal.
		 *
		 * @param[out] isValid Whether the data given by the user is
		 * valid.
		 */
		virtual void validityChanged (bool isValid) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LC::Azoth::IMUCJoinWidget,
		"org.Deviant.LeechCraft.Azoth.IMUCJoinWidget/1.0")

#endif
