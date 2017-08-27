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

#include "lcserviceoverride.h"

namespace LeechCraft
{
namespace Util
{
	namespace
	{
		QString GetHost (const ServiceInfo& info)
		{
			static const auto& env = qgetenv (info.EnvPrefix_ + "_SERVER_HOST");
			if (!env.isEmpty ())
				return env;

			return info.DefaultHost_;
		}

		int GetPort (const ServiceInfo& info)
		{
			static const auto& env = qgetenv (info.EnvPrefix_ + "_SERVER_PORT");
			if (!env.isEmpty ())
				return env.toInt ();

			return info.DefaultPort_;
		}

		QString GetProto (const ServiceInfo& info)
		{
			static const auto& env = qgetenv (info.EnvPrefix_ + "_NO_HTTPS");
			return env.isEmpty () && info.UseSslByDefault_ ? "https" : "http";
		}
	}

	QString GetServiceUrl (const ServiceInfo& info, const QString& path)
	{
		return QString { "%1://%2:%3/%4" }
				.arg (GetProto (info))
				.arg (GetHost (info))
				.arg (GetPort (info))
				.arg (path);
	}
}
}
