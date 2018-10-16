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

#include "ondiskstorage.h"
#include <QDir>
#include <QSqlError>
#include <util/db/oral/oral.h>
#include <util/sll/functor.h>
#include <util/sll/functional.h>
#include <util/sys/paths.h>
#include "entrystats.h"

namespace LeechCraft
{
namespace Azoth
{
namespace LastSeen
{
	struct OnDiskStorage::Record : EntryStats
	{
		Util::oral::PKey<QString, Util::oral::NoAutogen> EntryID_;

		Record () = default;

		Record (const QString& entryId, const EntryStats& stats)
		: EntryStats (stats)
		, EntryID_ (entryId)
		{
		}

		template<typename... Args>
		Record (const QString& entryId, Args&&... args)
		: EntryStats { std::forward<Args> (args)... }
		, EntryID_ { entryId }
		{
		}

		static QString ClassName ()
		{
			return "EntryStats";
		}
	};
}
}
}

using StatsRecord = LeechCraft::Azoth::LastSeen::OnDiskStorage::Record;

BOOST_FUSION_ADAPT_STRUCT (StatsRecord,
		EntryID_,
		Available_,
		Online_,
		StatusChange_)

namespace LeechCraft
{
namespace Azoth
{
namespace LastSeen
{
	namespace sph = Util::oral::sph;

	OnDiskStorage::OnDiskStorage (QObject *parent)
	: QObject { parent }
	, DB_ { QSqlDatabase::addDatabase ("QSQLITE",
				Util::GenConnectionName ("org.LeechCraft.Azoth.LastSeen.EntryStats")) }
	{
		const auto& cacheDir = Util::GetUserDir (Util::UserDir::Cache, "azoth/lastseen");
		DB_.setDatabaseName (cacheDir.filePath ("entrystats.db"));

		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		AdaptedRecord_ = Util::oral::AdaptPtr<Record> (DB_);
	}

	std::optional<EntryStats> OnDiskStorage::GetEntryStats (const QString& entryId)
	{
		return Util::Fmap (AdaptedRecord_->SelectOne (sph::f<&Record::EntryID_> == entryId),
				Util::Caster<EntryStats> {});
	}

	void OnDiskStorage::SetEntryStats (const QString& entryId, const EntryStats& stats)
	{
		AdaptedRecord_->Insert ({ entryId, stats }, Util::oral::InsertAction::Replace::PKey<Record>);
	}

	Util::DBLock OnDiskStorage::BeginTransaction ()
	{
		Util::DBLock lock { DB_ };
		lock.Init ();
		return lock;
	}
}
}
}
