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

#include <QWidget>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/iwkfontssettable.h>
#include "ui_mailtab.h"
#include "account.h"
#include "messageinfo.h"
#include "messagebodies.h"

class QStandardItemModel;
class QStandardItem;
class QSortFilterProxyModel;
class QToolButton;

namespace LeechCraft
{
namespace Util
{
	class ShortcutManager;
}

namespace Snails
{
	class MessageListEditorManager;
	class ComposeMessageTabFactory;
	class AccountsManager;
	class Storage;
	class MailTreeDelegate;
	class MailWebPage;

	enum class MsgType;

	class MailTab : public QWidget
				  , public ITabWidget
				  , public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget IWkFontsSettable)

		Ui::MailTab Ui_;

		MailWebPage *MailWebPage_;

		const ICoreProxy_ptr Proxy_;
		ComposeMessageTabFactory * const ComposeMessageTabFactory_;
		const AccountsManager * const AccsMgr_;
		Storage * const Storage_;

		QToolBar * const TabToolbar_;
		QToolBar * const MsgToolbar_;

		QMenu *MsgCopy_;
		QMenu *MsgMove_;

		QMenu *MsgAttachments_;
		QToolButton *MsgAttachmentsButton_;

		TabClassInfo TabClass_;
		QObject *PMT_;

		MessageListEditorManager *MsgListEditorMgr_;
		MailTreeDelegate *MailTreeDelegate_;

		MailListMode MailListMode_ = MailListMode::Normal;

		std::shared_ptr<MailModel> MailModel_;
		QSortFilterProxyModel *MailSortFilterModel_;
		Account_ptr CurrAcc_;
		std::optional<MessageInfo> CurrMsgInfo_;
		std::optional<MessageBodies> CurrMsgBodies_;

		std::shared_ptr<Account::FetchWholeMessageResult_t> CurrMsgFetchFuture_;

		bool HtmlViewAllowed_ = true;

		enum class MoveMessagesAction
		{
			Copy,
			Move
		};
	public:
		MailTab (const ICoreProxy_ptr&, const AccountsManager*, ComposeMessageTabFactory*, Storage*,
				const TabClassInfo&, Util::ShortcutManager*, QObject*, QWidget* = nullptr);

		static void FillShortcutsManager (Util::ShortcutManager*, const ICoreProxy_ptr&);

		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs ();
		void Remove ();
		QToolBar* GetToolBar () const;

		QObject* GetQObject ();
		void SetFontFamily (FontFamily, const QFont&);
		void SetFontSize (FontSize type, int size);
	private:
		void FillCommonActions (Util::ShortcutManager*);
		void FillMailActions (Util::ShortcutManager*);
		void MakeViewTypeButton ();
		void FillTabToolbarActions (Util::ShortcutManager*);

		QList<QByteArray> GetSelectedIds () const;

		void UpdateMsgActionsStatus ();
		void CheckFetchChildMessages (const QModelIndex&);
		QList<Folder> GetActualFolders () const;

		void SetMessage (const MessageInfo&, const std::optional<MessageBodies>&);

		void HandleLinkedRequested (MsgType);

		void SetHtmlViewAllowed (bool);

		void HandleAttachment (const QByteArray&, const QStringList&, const QString&);

		void PerformMoveMessages (const QList<QByteArray>&, const QList<QStringList>&, MoveMessagesAction);
		void PerformMoveMessages (const QList<QByteArray>&,
				const QStringList&, const QList<QStringList>&, MoveMessagesAction);
	private slots:
		void handleCurrentAccountChanged (const QModelIndex&);
		void handleCurrentTagChanged (const QModelIndex&);
		void handleMailSelected ();

		void rebuildOpsToFolders ();

		void handleCompose ();
		void handleCopyMultipleFolders ();
		void handleCopyMessages (QAction*);
		void handleMoveMultipleFolders ();
		void handleMoveMessages (QAction*);
		void handleMarkMsgRead ();
		void handleMarkMsgUnread ();
		void handleRemoveMsgs ();
		void handleViewHeaders ();
		void handleMultiSelect (bool);

		void selectAllChildren ();
		void expandAllChildren ();

		void deselectCurrent (const QList<QByteArray>& ids, const QStringList& folder);

		void handleFetchNewMail ();
		void handleRefreshFolder ();

		void on_TagsTree__customContextMenuRequested (const QPoint&);
	signals:
		void removeTab (QWidget*);

		void mailActionsEnabledChanged (bool);
	};
}
}
