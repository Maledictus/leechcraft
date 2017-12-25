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

#include "sslerrorschoicestorage.h"
#include <stdexcept>
#include <QDir>
#include <QSqlError>
#include <util/sys/paths.h>
#include <util/db/util.h>
#include <util/db/dblock.h>
#include <util/db/oral.h>
#include <util/db/migrate.h>
#include <util/sll/functor.h>

namespace LeechCraft
{
namespace Azoth
{
	struct SslErrorsChoiceStorage::Record
	{
		QByteArray AccountID_;
		QSslError::SslError Error_;
		SslErrorsChoiceStorage::Action Action_;

		using Constraints = Util::oral::Constraints<Util::oral::PrimaryKey<0, 1>>;

		static QString ClassName ()
		{
			return "SslErrors";
		}
	};
}
}

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Azoth::SslErrorsChoiceStorage::Record,
		AccountID_,
		Error_,
		Action_)

namespace LeechCraft
{
namespace Azoth
{
	SslErrorsChoiceStorage::SslErrorsChoiceStorage ()
	: DB_ { QSqlDatabase::addDatabase ("QSQLITE",
			Util::GenConnectionName ("org.LeechCraft.Azoth.SslErrors")) }
	{
		const auto& dbDir = Util::GetUserDir (Util::UserDir::LC, "azoth");
		DB_.setDatabaseName (dbDir.filePath ("sslerrors.db"));
		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		Util::oral::Migrate<Record> (DB_);

		AdaptedRecord_ = Util::oral::AdaptPtr<Record> (DB_);
	}

	namespace sph = Util::oral::sph;

	auto SslErrorsChoiceStorage::GetAction (const QByteArray& id,
			QSslError::SslError err) const -> boost::optional<Action>
	{
		using Util::operator*;

		return AdaptedRecord_->SelectOne (sph::_0 == id && sph::_1 == err) * &Record::Action_;
	}

	void SslErrorsChoiceStorage::SetAction (const QByteArray& id,
			QSslError::SslError err, Action act)
	{
		AdaptedRecord_->Insert ({ id, err, act }, Util::oral::InsertAction::Replace);
	}
}
}
