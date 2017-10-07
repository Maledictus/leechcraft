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
#include <QPointer>
#include <QPersistentModelIndex>
#include <QDateTime>
#include <util/sll/util.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/idndtab.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <interfaces/iwkfontssettable.h>
#include "interfaces/azoth/azothcommon.h"
#include "ui_chattab.h"

class QTextBrowser;
class QNetworkAccessManager;

namespace LeechCraft
{
namespace Util
{
	class FindNotificationWk;
}

namespace Azoth
{
	struct EntryStatus;
	class CoreMessage;
	class ICLEntry;
	class IMUCEntry;
	class IAccount;
	class IMessage;
	class ITransferManager;
	class ISupportPGP;
	class ContactDropFilter;
	class MsgFormatterWidget;
	class AvatarsManager;

	class ChatTab : public QWidget
				  , public ITabWidget
				  , public IRecoverableTab
				  , public IDNDTab
				  , public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget IRecoverableTab IDNDTab IWkFontsSettable)

		static QObject *S_ParentMultiTabs_;
		static TabClassInfo S_ChatTabClass_;
		static TabClassInfo S_MUCTabClass_;

		AvatarsManager * const AvatarsManager_;

		Ui::ChatTab Ui_;
		std::unique_ptr<QToolBar> TabToolbar_;
		QTextBrowser *MUCEventLog_;
		QAction *ToggleRichEditor_ = nullptr;
		QAction *ToggleRichText_ = nullptr;
		QAction *Call_ = nullptr;
#ifdef ENABLE_CRYPT
		QAction *EnableEncryption_ = nullptr;
#endif

		const QString EntryID_;

		QColor BgColor_;

		QList<QString> MsgHistory_;
		int CurrentHistoryPosition_ = -1;

		bool HadHighlight_ = false;
		int NumUnreadMsgs_ = 0;
		int ScrollbackPos_ = 0;

		QList<IMessage*> HistoryMessages_;
		QDateTime LastDateTime_;
		QList<CoreMessage*> CoreMessages_;

		QIcon TabIcon_;
		bool IsMUC_ = false;
		int PreviousTextHeight_ = 0;

		ContactDropFilter *CDF_;
		MsgFormatterWidget *MsgFormatter_ = nullptr;

		QString LastLink_;

		Util::FindNotificationWk *ChatFinder_;

		bool IsCurrent_ = false;

		QImage LastAvatar_;

		Util::DefaultScopeGuard AvatarChangeSubscription_;
	public:
		static void SetParentMultiTabs (QObject*);
		static void SetChatTabClassInfo (const TabClassInfo&);
		static void SetMUCTabClassInfo (const TabClassInfo&);

		static const TabClassInfo& GetChatTabClassInfo ();
		static const TabClassInfo& GetMUCTabClassInfo ();

		ChatTab (const QString&, AvatarsManager*, QNetworkAccessManager*, QWidget* = nullptr);
		~ChatTab ();

		/** Prepare (or update after it has been changed) the theme.
		 */
		void PrepareTheme ();

		/** Is called after the tab has been added to the tabwidget so
		 * that it could set its icon and various other stuff.
		 */
		void HasBeenAdded ();

		TabClassInfo GetTabClassInfo () const;
		QList<QAction*> GetTabBarContextMenuActions () const;
		QObject* ParentMultiTabs ();
		QToolBar* GetToolBar () const;
		void Remove ();
		void TabMadeCurrent ();
		void TabLostCurrent ();

		QByteArray GetTabRecoverData () const;
		QString GetTabRecoverName () const;
		QIcon GetTabRecoverIcon () const;

		void FillMimeData (QMimeData*);
		void HandleDragEnter (QDragMoveEvent*);
		void HandleDrop (QDropEvent*);

		QObject* GetQObject ();
		void SetFontFamily (FontFamily, const QFont&);
		void SetFontSize (FontSize type, int size);
		void SetFontSizeMultiplier (qreal factor);

		void ShowUsersList ();

		void HandleMUCParticipantsChanged ();

		void SetEnabled (bool);

		QObject* GetCLEntry () const;
		QString GetEntryID () const;
		QString GetSelectedVariant () const;

		QString ReformatTitle ();

		void ReinitEntry ();

		bool eventFilter (QObject*, QEvent*);
	public slots:
		void prepareMessageText (const QString&);
		void appendMessageText (const QString&);
		void insertMessageText (const QString&);
		void selectVariant (const QString&);
		QTextEdit* getMsgEdit ();

		void clearChat ();
	private slots:
		void on_MUCEventsButton__toggled (bool);
		void handleSeparateMUCLog (bool initial = false);

		void handleChatWindowSearch (const QString&);

		void handleEditScroll (int);
		void messageSend ();
		void on_MsgEdit__textChanged ();
		void on_SubjectButton__toggled (bool);
		void on_SubjChange__released ();
		void on_View__loadFinished (bool);
		void handleHistoryBack ();
		void handleRichEditorToggled ();
		void handleRichTextToggled ();
		void handleQuoteSelection ();
		void handleOpenLastLink ();
#ifdef ENABLE_MEDIACALLS
		void handleCallRequested ();
		void handleCall (QObject*);
#endif
#ifdef ENABLE_CRYPT
		void handleEnableEncryption ();
		void handleEncryptionStateChanged (QObject*, bool);
#endif
		void handleFileOffered (QObject*);
		void handleFileNoLongerOffered (QObject*);
		void handleOfferActionTriggered ();
		void handleEntryMessage (QObject*);
		void handleVariantsChanged (QStringList);
		void handleNameChanged (const QString& name);
		void handleStatusChanged (const EntryStatus&, const QString&);
		void handleChatPartStateChanged (const ChatPartState&, const QString&);
		void handleViewLinkClicked (QUrl, bool);
		void handleHistoryUp ();
		void handleHistoryDown ();

		void handleGotLastMessages (QObject*, const QList<QObject*>&);

		void handleSendButtonVisible ();
		void handleMinLinesHeightChanged ();
		void handleRichFormatterPosition ();

		void handleAccountStyleChanged (IAccount*);

		void performJS (const QString&);
	private:
		template<typename T>
		T* GetEntry () const;
		void BuildBasicActions ();
		void ReinitAvatar ();
		void CheckMUC ();
		void HandleMUC ();

		void InitExtraActions ();
		void AddManagedActions (bool first);

		void InitMsgEdit ();
		void RegisterSettings ();

		void RequestLogs (int);

		void UpdateTextHeight ();

		/** Appends the message to the message view area.
		 */
		void AppendMessage (IMessage*);

		/** Updates the tab icon and other usages of state icon from the
		 * TabIcon_.
		 */
		void UpdateStateIcon ();

		/** Insert nickname into message edit field.
		 * @param nickname a nick to insert, in html format.
		 */
		void InsertNick (const QString& nicknameHtml);

#ifdef ENABLE_CRYPT
		void SetEncryptionEnabled (ISupportPGP*, bool enable);
#endif
	signals:
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void needToClose (ChatTab*);
		void entryMadeCurrent (QObject*);
		void entryLostCurrent (QObject*);

		void tabRecoverDataChanged ();

		void composingTextChanged (const QString&);
		void currentVariantChanged (const QString&);

		// Hooks
		void hookChatTabCreated (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QObject *entry,
				QWebView *webView);
		void hookGonnaAppendMsg (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookMadeCurrent (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab);
		void hookMessageSendRequested (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QObject *entry,
				int type,
				QString variant);
		void hookThemeReloaded (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QWebView *view,
				QObject *entry);
	};

	typedef QPointer<ChatTab> ChatTab_ptr;
}
}
