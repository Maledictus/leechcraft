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

#include "requestbuilder.h"
#include <QUuid>

namespace LC
{
namespace Imgaste
{
	RequestBuilder::RequestBuilder ()
	{
		QString rnd = QUuid::createUuid ().toString ();
		rnd = rnd.mid (1, rnd.size () - 2);
		rnd += rnd;
		rnd = rnd.left (55);

		Boundary_ = "----------";
		Boundary_ += rnd;
	}

	void RequestBuilder::AddPair (const QString& name, const QString& value)
	{
		Built_.clear ();

		Result_ += "--";
		Result_ += Boundary_;
		Result_ += "\r\n";
		Result_ += "Content-Disposition: form-data; name=\"";
		Result_ += name.toLatin1 ();
		Result_ += "\"";
		Result_ += "\r\n\r\n";
		Result_ += value.toUtf8 ();
		Result_ += "\r\n";
	}

	void RequestBuilder::AddFile (const QString& format,
			const QString& name, const QByteArray& imageData)
	{
		Built_.clear ();

		Result_ += "--";
		Result_ += Boundary_;
		Result_ += "\r\n";
		Result_ += "Content-Disposition: form-data; name=\"";
		Result_ += name.toLatin1 ();
		Result_ += "\"; ";
		Result_ += "filename=\"";
		Result_ += QString ("screenshot.%1")
			.arg (format.toLower ())
			.toLatin1 ();
		Result_ += "\"";
		Result_ += "\r\n";
		Result_ += "Content-Type: ";
		if (format.toLower () == "jpg")
			Result_ += "image/jpeg";
		else
			Result_ += "image/png";
		Result_ += "\r\n\r\n";

		Result_ += imageData;
		Result_ += "\r\n";
	}

	QByteArray RequestBuilder::Build () const
	{
		if (!Built_.isEmpty ())
			return Built_;

		Built_ = Result_;

		Built_ += "--";
		Built_ += Boundary_;
		Built_ += "--";

		return Built_;
	}

	QString RequestBuilder::GetBoundary () const
	{
		return Boundary_;
	}
}
}
