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

#include "shx.h"
#include <QProcess>
#include <QtDebug>
#include <QTextDocument>
#include <QMessageBox>
#include <util/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace SHX
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("azoth_shx");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "azothshxsettings.xml");

		if (XmlSettingsManager::Instance ().property ("Command").toString ().isEmpty ())
		{
#ifdef Q_OS_WIN32
			const QString& cmd = "cmd.exe /U /S";
#else
			const QString& cmd = "/bin/sh -c";
#endif
			XmlSettingsManager::Instance ().setProperty ("Command", cmd);
		}
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.SHX";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Azoth SHX";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Allows one to execute arbitrary shell commands and paste their result.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/azoth/shx/resources/images/shx.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	void Plugin::hookMessageWillCreated (LeechCraft::IHookProxy_ptr proxy,
			QObject *chatTab, QObject*, int, QString)
	{
		QString text = proxy->GetValue ("text").toString ();

		const QString marker = XmlSettingsManager::Instance ()
				.property ("Marker").toString ();
		if (!text.startsWith (marker))
			return;

		text = text.mid (marker.size ());
		if (XmlSettingsManager::Instance ().property ("WarnAboutExecution").toBool ())
		{
			const auto& msgText = tr ("Are you sure you want to execute this command?") +
					"<blockquote><em>" + Qt::escape (text) + "</em></blockquote>";
			if (QMessageBox::question (0, "LeechCraft",
						msgText,
						QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
				return;
		}

		proxy->CancelDefault ();

		auto proc = new QProcess ();
		Process2Chat_ [proc] = chatTab;
		connect (proc,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handleFinished ()));

		const auto& commandParts = XmlSettingsManager::Instance ()
				.property ("Command").toString ().split (" ", QString::SkipEmptyParts);
		const auto& command = commandParts.value (0);
		if (command.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "empty command";
			return;
		}
		proc->start (command, commandParts.mid (1) << text);
	}

	void Plugin::handleFinished ()
	{
		auto proc = qobject_cast<QProcess*> (sender ());
		proc->deleteLater ();

		if (!Process2Chat_.contains (proc))
		{
			qWarning () << Q_FUNC_INFO
					<< "no chat for process"
					<< proc;
			return;
		}
#ifdef Q_OS_WIN32
		auto out = QString::fromUtf16 (reinterpret_cast<const ushort*> (proc->readAllStandardOutput ().constData ()));
#else
		auto out = QString::fromUtf8 (proc->readAllStandardOutput ());
#endif
		const auto& err = proc->readAllStandardError ();

		if (!err.isEmpty ())
#ifdef Q_OS_WIN32
			out.prepend (tr ("Error: %1").arg (QString::fromUtf16 (reinterpret_cast<const ushort*> (err.constData ()))) + "\n");
#else
			out.prepend (tr ("Error: %1").arg (QString::fromUtf8 (err)) + "\n");
#endif

		QMetaObject::invokeMethod (Process2Chat_.take (proc),
				"prepareMessageText",
				Q_ARG (QString, out));
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_shx, LeechCraft::Azoth::SHX::Plugin);
