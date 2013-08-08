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

#ifndef PLUGINS_AUSCRIE_AUSCRIE_H
#define PLUGINS_AUSCRIE_AUSCRIE_H
#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/iactionsexporter.h>

namespace LeechCraft
{
namespace Auscrie
{
	class ShooterDialog;

	class Plugin : public QObject
					, public IInfo
					, public IActionsExporter
	{
		Q_OBJECT
#ifdef USE_QT5
		Q_PLUGIN_METADATA (IID "org.LeechCraft.Auscrie");
#endif
		Q_INTERFACES (IInfo IActionsExporter)

		ICoreProxy_ptr Proxy_;
		QAction *ShotAction_;
		ShooterDialog *Dialog_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		QList<QAction*> GetActions (ActionsEmbedPlace) const;
	private slots:
		void makeScreenshot ();
		void shoot ();
	private:
		QPixmap GetPixmap () const;
		void Post (const QByteArray&);
	signals:
		void gotEntity (const LeechCraft::Entity&);

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);
	};
}
}

#endif
