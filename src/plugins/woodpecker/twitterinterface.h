/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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
#include <QList>
#include <QObject>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QtKOAuth/QtKOAuth>
#include "tweet.h"

namespace LeechCraft
{
namespace Woodpecker
{
	enum class TwitterRequest
	{
		HomeTimeline,
		Mentions,
		UserTimeline,
		Update,
		Direct,
		Retweet,
		Reply,
		SpamReport,
	};

	enum class FeedMode
	{
		HomeTimeline,
		Mentions,
		UserTimeline,
		SearchResult,
		Direct,
	};

	class TwitterInterface : public QObject
	{
		Q_OBJECT
	
		QNetworkAccessManager *HttpClient_;
		QNetworkReply *Reply_;
		KQOAuthManager *OAuthManager_;
		KQOAuthRequest *OAuthRequest_;
		QString Token_;
		QString TokenSecret_;
		QString ConsumerKey_;
		QString ConsumerKeySecret_;
		QSettings *Settings_;
		FeedMode LastRequestMode_;
		
		void SignedRequest (TwitterRequest req, KQOAuthRequest::RequestHttpMethod method = KQOAuthRequest::GET, KQOAuthParameters params = KQOAuthParameters ());
		void RequestTwitter (const QUrl& requestAddress);
		QList <std::shared_ptr<Tweet>> ParseReply (const QByteArray& json);
		void Xauth ();
		
	public:
		explicit TwitterInterface (QObject *parent = 0);
		~TwitterInterface ();
		void SendTweet (const QString& tweet);
		void Retweet (const qulonglong id);
		void Reply (const qulonglong replyid, const QString& tweet);
		void ReportSPAM (const QString& username, const qulonglong userid=0);
		void GetAccess ();
		void Login (const QString& savedToken, const QString& savedTokenSecret);
		FeedMode GetLastRequestMode ();
		void SetLastRequestMode (const FeedMode& newLastRequestMode);
		
	private slots:
		void replyFinished ();
		
		void onTemporaryTokenReceived (const QString& temporaryToken, const QString& temporaryTokenSecret);
		void onAuthorizationReceived (const QString& token, const QString& verifier);
		void onRequestReady (const QByteArray&);
		void onAuthorizedRequestDone ();
		void onAccessTokenReceived (const QString& token, const QString& tokenSecret);
		
	signals:
		void tweetsReady (const QList<std::shared_ptr<Tweet>>&);
		void authorized (const QString&, const QString&);
		
	public slots:
		void requestHomeFeed ();
		void searchTwitter (const QString& text);
		void requestUserTimeline (const QString& username);
		void requestMoreTweets (const QString& last);
	};
}
}

