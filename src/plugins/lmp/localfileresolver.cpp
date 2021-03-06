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

#include "localfileresolver.h"
#include <QtDebug>
#include <QFileInfo>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <util/sll/prelude.h>
#include <util/sll/either.h>
#include "util/lmp/gstutil.h"
#include "xmlsettingsmanager.h"

namespace LC
{
namespace LMP
{
	TagLib::FileRef LocalFileResolver::GetFileRef (const QString& file) const
	{
#ifdef Q_OS_WIN32
		return TagLib::FileRef (reinterpret_cast<const wchar_t*> (file.utf16 ()));
#else
		return TagLib::FileRef (file.toUtf8 ().constData (), true, TagLib::AudioProperties::Accurate);
#endif
	}

	LocalFileResolver::ResolveResult_t LocalFileResolver::ResolveInfo (const QString& file)
	{
		const auto& modified = QFileInfo (file).lastModified ();

		{
			QReadLocker locker (&CacheLock_);
			if (Cache_.contains (file))
			{
				const auto& pair = Cache_ [file];
				if (pair.first == modified)
					return ResolveResult_t::Right (pair.second);
			}
		}

		QMutexLocker tlLocker (&TaglibMutex_);

		auto r = GetFileRef (file);
		auto tag = r.tag ();
		if (!tag)
			return ResolveResult_t::Left ({ file, "cannot get audio tags" });

		auto audio = r.audioProperties ();

		auto& xsm = XmlSettingsManager::Instance ();
		const auto& region = xsm.property ("EnableLocalTagsRecoding").toBool () ?
				xsm.property ("TagsRecodingRegion").toString () :
				QString {};
		auto ftl = [&region] (const TagLib::String& str)
		{
			return GstUtil::FixEncoding (QString::fromUtf8 (str.toCString (true)), region);
		};

		const auto& genres = ftl (tag->genre ()).split ('/', QString::SkipEmptyParts);

		MediaInfo info
		{
			file,
			ftl (tag->artist ()),
			ftl (tag->album ()),
			ftl (tag->title ()),
			Util::Map (genres, [] (const QString& genre) { return genre.trimmed (); }),
			audio ? audio->length () : 0,
			static_cast<qint32> (tag->year ()),
			static_cast<qint32> (tag->track ())
		};
		{
			QWriteLocker locker (&CacheLock_);
			if (Cache_.size () > 200)
				Cache_.clear ();
			Cache_ [file] = qMakePair (modified, info);
		}
		return ResolveResult_t::Right (info);
	}

	QMutex& LocalFileResolver::GetMutex ()
	{
		return TaglibMutex_;
	}

	void LocalFileResolver::flushCache ()
	{
		QWriteLocker locker { &CacheLock_ };
		Cache_.clear ();
	}
}
}
