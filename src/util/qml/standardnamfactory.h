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

#include <functional>
#include <QtGlobal>
#include <QQmlNetworkAccessManagerFactory>
#include "qmlconfig.h"

class QQmlEngine;

namespace LC
{
namespace Util
{
	/** @brief A standard QML QNetworkAccessManager factory.
	 *
	 * StandardNAMFactory allows easily creating QNetworkAccessManager
	 * instances in QML contexts.
	 *
	 * The created managers are all using the same cache, located at the
	 * cache path passed and limited by the maximum size passed to the
	 * constructor.
	 *
	 * Several different factories may be created sharing the same cache
	 * location. In this case, the minimum value of the cache size would
	 * be used as the maximum.
	 *
	 * @ingroup QmlUtil
	 */
	class UTIL_QML_API StandardNAMFactory : public QQmlNetworkAccessManagerFactory
	{
		const QString Subpath_;
	public:
		/** @brief The type of the function used to query the cache size
		 * by the factory.
		 */
		using CacheSizeGetter_f = std::function<int ()>;
	private:
		CacheSizeGetter_f CacheSizeGetter_;
	public:
		/** @brief Constructs a new StandardNAMFactory.
		 *
		 * The cache uses a subdirectory \em subpath in the \em network
		 * directory of the user cache location.
		 *
		 * @param[in] subpath The subpath in cache user location.
		 * @param[in] getter The function that would be queried during
		 * periodical cache garbage collection to fetch the current
		 * maximum cache size.
		 * @param[in] engine The QML engine where this factory should be
		 * installed, if not null.
		 */
		StandardNAMFactory (const QString& subpath,
				CacheSizeGetter_f getter,
				QQmlEngine *engine = nullptr);

		/** @brief Creates the network access manager with the given
		 * \em parent.
		 *
		 * This function implements a pure virtual in Qt's base factory
		 * class (QDeclarativeNetworkAccessManagerFactory for Qt4 or
		 * QQmlNetworkAccessManagerFactory for Qt5).
		 *
		 * The ownership of the returned QNetworkAccessManager is passed
		 * to the caller.
		 *
		 * @param[in] parent The parent of the QNetworkAccessManager to
		 * be created.
		 * @return A new QNetworkAccessManager.
		 */
		QNetworkAccessManager* create (QObject *parent);
	};
}
}
