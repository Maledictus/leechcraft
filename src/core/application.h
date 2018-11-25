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

#include <boost/program_options.hpp>
#include <QApplication>
#include <QStringList>

namespace LeechCraft
{
	class SplashScreen;

	/** Manages the main application-level behavior of LeechCraft like
	 * single-instance feature, communication with Session Manager,
	 * restarting, parsing command line and such.
	 */
	class Application : public QApplication
	{
		Q_OBJECT

		QStringList Arguments_;

		const QString DefaultSystemStyleName_;
		QString PreviousLangName_;

		boost::program_options::variables_map VarMap_;
		bool CatchExceptions_ = true;

		SplashScreen *Splash_ = nullptr;
	public:
		enum Errors
		{
			EAlreadyRunning = 1,
			EPaths = 2,
			EHelpRequested = 3,
			EGeneralSocketError = 4,
			EVersionRequested = 5
		};

		/** Constructs the Application, parses the command line,
		 * installs Qt-wide translations, performs some basic checks
		 * and registers commonly used meta types.
		 *
		 * @param[in] argc argc from main().
		 * @param[in] argv argcvfrom main().
		 */
		Application (int& argc, char **argv);

		/** Returns the cached copy of QCoreApplication::arguments().
		 * Provided for performance reasons, as Qt docs say that calling
		 * the original function is slow.
		 *
		 * @return Cached copy of QCoreApplication::arguments().
		 */
		const QStringList& Arguments () const;

		boost::program_options::variables_map Parse (boost::program_options::wcommand_line_parser& parser,
				boost::program_options::options_description *desc) const;
		const boost::program_options::variables_map& GetVarMap () const;

		/** Returns the local socket name based on the user name/id and
		 * such things.
		 *
		 * @return String with the socket name.
		 */
		static QString GetSocketName ();

		/** Returns the splash screen during LeechCraft startup.
		 *
		 * After finishInit() is invoked, the return value of this
		 * function is undefined.
		 */
		SplashScreen* GetSplashScreen () const;

		/** Performs restart: starts a detached copy with '-restart'
		 * switch and calls qApp->quit().
		 */
		void InitiateRestart ();

		void Quit ();

		/** Checks whether another instance of LeechCraft is running.
		 */
		bool IsAlreadyRunning () const;

		/** Overloaded QApplication::notify() provided to catch exceptions
		 * in slots.
		 */
		virtual bool notify (QObject*, QEvent*);
	private:
		/** Parses command line and sets corresponding application-wide
		 * options.
		 */
		void ParseCommandLine ();

		void InitPluginsIconset ();

		/** Enter the restart mode. This is called in case leechcraft is
		 * started with the '-restart' option.
		 */
		void EnterRestartMode ();

		void CheckStartupPass ();
		void InitSettings ();

		void InitSessionManager ();
	private slots:
		void finishInit ();

		void handleAppStyle ();
		void handleLanguage ();

		/** Checks whether another copy of LeechCraft is still running
		 * via a call to IsAlreadyRunning(), and if it isn't, starts a
		 * new leechcraft process with the corresponding arguments.
		 */
		void checkStillRunning ();
	};
};
