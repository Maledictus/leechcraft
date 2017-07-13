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

#include <QSortFilterProxyModel>

namespace LeechCraft
{
namespace Azoth
{
	class SortFilterProxyModel : public QSortFilterProxyModel
	{
		Q_OBJECT

		bool ShowOffline_ = true;
		bool MUCMode_ = false;
		bool OrderByStatus_ = true;
		bool HideMUCParts_ = false;
		bool ShowSelfContacts_ = true;
		QObject *MUCEntry_ = nullptr;
	public:
		SortFilterProxyModel (QObject* = nullptr);

		void SetMUCMode (bool);
		bool IsMUCMode () const;
		void SetMUC (QObject*);
	public slots:
		void showOfflineContacts (bool);
	private slots:
		void handleStatusOrderingChanged ();
		void handleHideMUCPartsChanged ();
		void handleShowSelfContactsChanged ();
		void handleMUCDestroyed ();
	protected:
		bool filterAcceptsRow (int, const QModelIndex&) const override;
		bool lessThan (const QModelIndex&, const QModelIndex&) const override;
	signals:
		void mucMode ();
		void wholeMode ();
	};
}
}
