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

#include "sslerrorshandler.h"
#include <QSslError>
#include <QNetworkReply>
#include <QSettings>
#include <QCoreApplication>
#include <QPointer>
#include <QtDebug>
#include <util/sll/slotclosure.h>
#include "sslerrorsdialog.h"

namespace LeechCraft
{
	SslErrorsHandler::SslErrorsHandler (QNetworkReply *reply,
			const QList<QSslError>& errors, QObject *parent)
	: QObject { parent }
	{
		const std::shared_ptr<QSettings> settings
		{
			new QSettings
			{
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName ()

			},
			[] (QSettings *settings)
			{
				settings->endGroup ();
				delete settings;
			}
		};
		settings->beginGroup ("SSL exceptions");
		const auto& keys = settings->allKeys ();
		const auto& url = reply->url ();
		const auto& urlString = url.toString ();
		const auto& host = url.host ();

		if (keys.contains (urlString))
		{
			if (settings->value (urlString).toBool ())
				reply->ignoreSslErrors ();

			return;
		}
		else if (keys.contains (host))
		{
			if (settings->value (host).toBool ())
				reply->ignoreSslErrors ();

			return;
		}

		QString msg = tr ("<code>%1</code><br />has SSL errors."
				" What do you want to do?")
			.arg (QApplication::fontMetrics ().elidedText (urlString, Qt::ElideMiddle, 300));

		auto errDialog = new SslErrorsDialog;
		errDialog->setAttribute (Qt::WA_DeleteOnClose);
		errDialog->Update (msg, errors);
		errDialog->open ();

		const auto contSync = 0;
		const auto contAsync = 1;

		QEventLoop loop;
		Util::SlotClosure<Util::NoDeletePolicy> replyExitGuard
		{
			[&loop, reply]
			{
				qDebug () << Q_FUNC_INFO
						<< "reply died, gonna exit"
						<< reply->error ();
				if (loop.isRunning ())
					loop.exit (contAsync);
			},
			reply,
			SIGNAL (finished ()),
			this
		};
		Util::SlotClosure<Util::NoDeletePolicy> dialogExitGuard
		{
			[&loop]
			{
				qDebug () << Q_FUNC_INFO
						<< "dialog accepted";
				if (loop.isRunning ())
					loop.exit (contSync);
			},
			errDialog,
			SIGNAL (finished (int)),
			this
		};

		const auto finishedHandler = [=]
				{
					HandleFinished (errDialog->result (), errDialog->GetRememberChoice (),
							urlString, host, settings);
				};

		if (loop.exec () == contAsync)
		{
			new Util::SlotClosure<Util::NoDeletePolicy>
			{
				finishedHandler,
				errDialog,
				SIGNAL (finished (int)),
				this
			};

			return;
		}
		finishedHandler ();

		switch (reply->error ())
		{
		case QNetworkReply::SslHandshakeFailedError:
			qWarning () << Q_FUNC_INFO
					<< "got SSL handshake error in handleSslErrors, but let's try to continue";
		case QNetworkReply::NoError:
			break;
		default:
			return;
		}

		if (errDialog->result () == QDialog::Accepted)
			reply->ignoreSslErrors ();
	}

	void SslErrorsHandler::HandleFinished (int result,
				SslErrorsDialog::RememberChoice rc,
				const QString& urlString,
				const QString& host,
				const std::shared_ptr<QSettings>& settings)
	{
		const bool ignore = result == QDialog::Accepted;

		switch (rc)
		{
		case SslErrorsDialog::RCFile:
			settings->setValue (urlString, ignore);
			break;
		case SslErrorsDialog::RCHost:
			settings->setValue (host, ignore);
			break;
		default:
			break;
		}

	}
}
