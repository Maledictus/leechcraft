/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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
#include <QHash>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/itexteditor.h>
#include "ui_richeditorwidget.h"

class QToolBar;

namespace LeechCraft
{
namespace LHTR
{
	class RichEditorWidget : public QWidget
						   , public IEditorWidget
						   , public IAdvancedHTMLEditor
	{
		Q_OBJECT
		Q_INTERFACES (IEditorWidget IAdvancedHTMLEditor)

		ICoreProxy_ptr Proxy_;
		Ui::RichEditorWidget Ui_;

		QToolBar *ViewBar_;

		QHash<QWebPage::WebAction, QAction*> WebAction2Action_;
		QHash<QString, QHash<QString, QAction*>> Cmd2Action_;

		Replacements_t Rich2HTML_;
		Replacements_t HTML2Rich_;

		bool HTMLDirty_;

		QAction *FindAction_;
		QAction *ReplaceAction_;
	public:
		RichEditorWidget (ICoreProxy_ptr, QWidget* = 0);

		QString GetContents (ContentType) const;
		void SetContents (const QString&, ContentType);
		void AppendAction (QAction*);
		void AppendSeparator ();
		void RemoveAction (QAction*);
		QAction* GetEditorAction (EditorAction);
		void SetBackgroundColor (const QColor&, ContentType);

		void InsertHTML (const QString&);
		void SetTagsMappings (const Replacements_t&, const Replacements_t&);
		void ExecJS (const QString&);

		bool eventFilter (QObject*, QEvent*);
	private:
		void InternalSetBgColor (const QColor&, ContentType);

		void SetupTableMenu ();

		void ExecCommand (const QString&, const QString& = QString ());
		bool QueryCommandState (const QString& cmd);

		void OpenFindReplace (bool findOnly);
	private slots:
		void handleBgColorSettings ();

		void handleLinkClicked (const QUrl&);
		void on_TabWidget__currentChanged (int);

		void setupJS ();

		void on_HTML__textChanged ();
		void updateActions ();

		void handleCmd ();
		void handleInlineCmd ();
		void handleBgColor ();
		void handleFgColor ();
		void handleFont ();

		void handleInsertTable ();
		void handleInsertRow ();
		void handleInsertColumn ();
		void handleRemoveRow ();
		void handleRemoveColumn ();

		void handleInsertLink ();
		void handleInsertImage ();

		void handleFind ();
		void handleReplace ();
	signals:
		void textChanged ();
	};
}
}
