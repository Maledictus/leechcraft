/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <memory>
#include <qwebpage.h>
#include <QUrl>
#include <QNetworkRequest>
#include <interfaces/structures.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/core/icoreproxyfwd.h>
#include <interfaces/poshuku/poshukutypes.h>
#include <interfaces/poshuku/ilinkopenmodifier.h>

namespace LC
{
namespace Poshuku
{
class IProxyObject;

namespace WebKitView
{
	class JSProxy;
	class ExternalProxy;
	class CustomWebView;

	class CustomWebPage : public QWebPage
	{
		Q_OBJECT

		const ICoreProxy_ptr Proxy_;
		IProxyObject * const PoshukuProxy_;

		QUrl LoadingURL_;
		std::shared_ptr<JSProxy> JSProxy_;
		std::shared_ptr<ExternalProxy> ExternalProxy_;
		PageFormsData_t FilledState_;

		QMap<ErrorDomain, QMap<int, QStringList>> Error2Suggestions_;

		const ILinkOpenModifier_ptr LinkOpenModifier_;
	public:
		CustomWebPage (const ICoreProxy_ptr&, IProxyObject*, QObject* = nullptr);

		void HandleViewReady ();

		QUrl GetLoadingURL () const;

		bool supportsExtension (Extension) const override;
		bool extension (Extension, const ExtensionOption*, ExtensionReturn*) override;
	private slots:
		void handleContentsChanged ();
		void handleDatabaseQuotaExceeded (QWebFrame*, QString);
		void handleDownloadRequested (QNetworkRequest);
		void handleFrameCreated (QWebFrame*);
		void handleJavaScriptWindowObjectCleared ();
		void handleGeometryChangeRequested (const QRect&);
		void handleLinkClicked (const QUrl&);
		void handleLinkHovered (const QString&, const QString&, const QString&);
		void handleLoadFinished (bool);
		void handleUnsupportedContent (QNetworkReply*);
		void handleWindowCloseRequested ();
		void fillForms (QWebFrame*);
	protected:
		bool acceptNavigationRequest (QWebFrame*, const QNetworkRequest&, QWebPage::NavigationType) override;
		QString chooseFile (QWebFrame*, const QString&) override;
		QObject* createPlugin (const QString&, const QUrl&, const QStringList&, const QStringList&) override;
		QWebPage* createWindow (WebWindowType) override;
		void javaScriptAlert (QWebFrame*, const QString&) override;
		bool javaScriptConfirm (QWebFrame*, const QString&) override;
		void javaScriptConsoleMessage (const QString&, int, const QString&) override;
		bool javaScriptPrompt (QWebFrame*, const QString&, const QString&, QString*) override;
		QString userAgentForUrl (const QUrl&) const override;
	private:
		bool HandleExtensionProtocolUnknown (const ErrorPageExtensionOption*);
		void FillErrorSuggestions ();
		QString MakeErrorReplyContents (int, const QUrl&,
				const QString&, ErrorDomain = WebKit) const;
		QWebFrame* FindFrame (const QUrl&);
		void HandleForms (QWebFrame*, const QNetworkRequest&,
				QWebPage::NavigationType);
	signals:
		void loadingURL (const QUrl&);
		void storeFormData (const PageFormsData_t&);
		void delayedFillForms (QWebFrame*);

		void webViewCreated (const std::shared_ptr<CustomWebView>&, bool);

		// Hook support signals
		void hookAcceptNavigationRequest (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frame,
				QNetworkRequest request,
				QWebPage::NavigationType type);
		void hookChooseFile (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frame,
				QString suggested);
		void hookContentsChanged (LC::IHookProxy_ptr proxy,
				QWebPage *page);
		void hookCreatePlugin (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QString clsid,
				QUrl url,
				QStringList params,
				QStringList values);
		void hookCreateWindow (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebPage::WebWindowType type);
		void hookDatabaseQuotaExceeded (LC::IHookProxy_ptr proxy,
				QWebPage *sourcePage,
				QWebFrame *sourceFrame,
				QString databaseName);
		void hookDownloadRequested (LC::IHookProxy_ptr proxy,
				QWebPage *sourcePage,
				QNetworkRequest downloadRequest);
		void hookExtension (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebPage::Extension extension,
				const QWebPage::ExtensionOption* extensionOption,
				QWebPage::ExtensionReturn* extensionReturn);
		void hookFrameCreated (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frameCreated);
		void hookGeometryChangeRequested (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QRect rect);
		void hookJavaScriptAlert (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frame,
				QString msg);
		void hookJavaScriptConfirm (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frame,
				QString msg);
		void hookJavaScriptConsoleMessage (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QString msg,
				int line,
				QString sourceId);
		void hookJavaScriptPrompt (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QWebFrame *frame,
				QString msg,
				QString defValue,
				QString resultString);
		void hookJavaScriptWindowObjectCleared (LC::IHookProxy_ptr proxy,
				QWebPage *sourcePage,
				QWebFrame *frameCleared);
		void hookLinkClicked (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QUrl url);
		void hookLinkHovered (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QString link,
				QString title,
				QString textContent);
		void hookLoadFinished (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				bool result);
		void hookSupportsExtension (LC::IHookProxy_ptr proxy,
				const QWebPage *page,
				QWebPage::Extension extension) const;
		void hookUnsupportedContent (LC::IHookProxy_ptr proxy,
				QWebPage *page,
				QNetworkReply *reply);
		void hookWebPageConstructionBegin (LC::IHookProxy_ptr proxy,
				QWebPage *page);
		void hookWebPageConstructionEnd (LC::IHookProxy_ptr proxy,
				QWebPage *page);
		void hookWindowCloseRequested (LC::IHookProxy_ptr proxy,
				QWebPage *page);
	};
}
}
}
