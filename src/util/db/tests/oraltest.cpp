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

#include "oraltest.h"
#include "common.h"

QTEST_APPLESS_MAIN (LeechCraft::Util::OralTest)

struct SimpleRecord
{
	lco::PKey<int, lco::NoAutogen> ID_;
	QString Value_;

	static QString ClassName ()
	{
		return "SimpleRecord";
	}

	auto AsTuple () const
	{
		return std::tie (ID_, Value_);
	}
};

BOOST_FUSION_ADAPT_STRUCT (SimpleRecord,
		ID_,
		Value_)

TOSTRING (SimpleRecord)

struct AutogenPKeyRecord
{
	lco::PKey<int> ID_;
	QString Value_;

	static QString ClassName ()
	{
		return "AutogenPKeyRecord";
	}

	auto AsTuple () const
	{
		return std::tie (ID_, Value_);
	}
};

BOOST_FUSION_ADAPT_STRUCT (AutogenPKeyRecord,
		ID_,
		Value_)

TOSTRING (AutogenPKeyRecord)

struct NoPKeyRecord
{
	int ID_;
	QString Value_;

	static QString ClassName ()
	{
		return "NoPKeyRecord";
	}

	auto AsTuple () const
	{
		return std::tie (ID_, Value_);
	}
};

BOOST_FUSION_ADAPT_STRUCT (NoPKeyRecord,
		ID_,
		Value_)

TOSTRING (NoPKeyRecord)

struct NonInPlaceConstructibleRecord
{
	int ID_;
	QString Value_;

	NonInPlaceConstructibleRecord () = default;

	NonInPlaceConstructibleRecord (int id, const QString& value, double someExtraArgument)
	: ID_ { id }
	, Value_ { value }
	{
		Q_UNUSED (someExtraArgument)
	}

	static QString ClassName ()
	{
		return "NonInPlaceConstructibleRecord";
	}

	auto AsTuple () const
	{
		return std::tie (ID_, Value_);
	}
};

BOOST_FUSION_ADAPT_STRUCT (NonInPlaceConstructibleRecord,
		ID_,
		Value_)

TOSTRING (NonInPlaceConstructibleRecord)

struct ComplexConstraintsRecord
{
	int ID_;
	QString Value_;
	int Age_;
	int Weight_;

	static QString ClassName ()
	{
		return "ComplexConstraintsRecord";
	}

	auto AsTuple () const
	{
		return std::tie (ID_, Value_, Age_, Weight_);
	}

	using Constraints = lco::Constraints<
				lco::PrimaryKey<0, 1>,
				lco::UniqueSubset<2, 3>
			>;
};

BOOST_FUSION_ADAPT_STRUCT (ComplexConstraintsRecord,
		ID_,
		Value_,
		Age_,
		Weight_)

TOSTRING (ComplexConstraintsRecord)

namespace LeechCraft
{
namespace Util
{
	namespace
	{
		template<typename T>
		auto PrepareRecords (QSqlDatabase db)
		{
			auto adapted = Util::oral::AdaptPtr<T, OralFactory> (db);
			for (int i = 0; i < 3; ++i)
				adapted->Insert ({ i, QString::number (i) });
			return adapted;
		}
	}

	namespace sph = oral::sph;

	void OralTest::testSimpleRecordInsertSelect ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<SimpleRecord> { { 0, "0" }, { 1, "1" }, { 2, "2" } }));
	}

	void OralTest::testSimpleRecordInsertReplaceSelect ()
	{
		auto db = MakeDatabase ();

		auto adapted = Util::oral::AdaptPtr<SimpleRecord, OralFactory> (db);
		for (int i = 0; i < 3; ++i)
			adapted->Insert ({ 0, QString::number (i) }, lco::InsertAction::Replace::PKey<SimpleRecord>);

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<SimpleRecord> { { 0, "2" } }));
	}

	void OralTest::testSimpleRecordInsertIgnoreSelect ()
	{
		auto db = MakeDatabase ();

		auto adapted = Util::oral::AdaptPtr<SimpleRecord, OralFactory> (db);
		for (int i = 0; i < 3; ++i)
			adapted->Insert ({ 0, QString::number (i) }, lco::InsertAction::Ignore);

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<SimpleRecord> { { 0, "0" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByPos ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::_0 == 1);
		QCOMPARE (list, (QList<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByPos2 ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::_0 < 2);
		QCOMPARE (list, (QList<SimpleRecord> { { 0, "0" }, { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByPos3 ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::_0 < 2 && sph::_1 == QString { "1" });
		QCOMPARE (list, (QList<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectOneByPos ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& single = adapted->SelectOne (sph::_0 == 1);
		QCOMPARE (single, (boost::optional<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByFields ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::f<&SimpleRecord::ID_> == 1);
		QCOMPARE (list, (QList<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByFields2 ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::f<&SimpleRecord::ID_> < 2);
		QCOMPARE (list, (QList<SimpleRecord> { { 0, "0" }, { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectByFields3 ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::f<&SimpleRecord::ID_> < 2 && sph::f<&SimpleRecord::Value_> == QString { "1" });
		QCOMPARE (list, (QList<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectOneByFields ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& single = adapted->SelectOne (sph::f<&SimpleRecord::ID_> == 1);
		QCOMPARE (single, (boost::optional<SimpleRecord> { { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectSingleFieldByFields ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::fields<&SimpleRecord::Value_>, sph::f<&SimpleRecord::ID_> < 2);
		QCOMPARE (list, (QList<QString> { "0", "1" }));
	}

	void OralTest::testSimpleRecordInsertSelectFieldsByFields ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto& list = adapted->Select (sph::fields<&SimpleRecord::ID_, &SimpleRecord::Value_>, sph::f<&SimpleRecord::ID_> < 2);
		QCOMPARE (list, (QList<std::tuple<int, QString>> { { 0, "0" }, { 1, "1" } }));
	}

	void OralTest::testSimpleRecordInsertSelectCount ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto count = adapted->Select (sph::count);
		QCOMPARE (count, 3);
	}

	void OralTest::testSimpleRecordInsertSelectCountByFields ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		const auto count = adapted->Select (sph::count, sph::f<&SimpleRecord::ID_> < 2);
		QCOMPARE (count, 2);
	}

	void OralTest::testSimpleRecordUpdate ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		adapted->Update ({ 0, "meh" });
		const auto updated = adapted->Select (sph::f<&SimpleRecord::ID_> == 0);
		QCOMPARE (updated, (QList<SimpleRecord> { { 0, "meh" } }));
	}

	void OralTest::testSimpleRecordUpdateExprTree ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		adapted->Update (sph::f<&SimpleRecord::Value_> = QString { "meh" }, sph::f<&SimpleRecord::ID_> == 0);
		const auto updated = adapted->Select (sph::f<&SimpleRecord::ID_> == 0);
		QCOMPARE (updated, (QList<SimpleRecord> { { 0, "meh" } }));
	}

	void OralTest::testSimpleRecordUpdateMultiExprTree ()
	{
		auto adapted = PrepareRecords<SimpleRecord> (MakeDatabase ());
		adapted->Update ((sph::f<&SimpleRecord::Value_> = QString { "meh" }, sph::f<&SimpleRecord::ID_> = 10),
				sph::f<&SimpleRecord::ID_> == 0);
		const auto updated = adapted->Select (sph::f<&SimpleRecord::ID_> == 10);
		QCOMPARE (updated, (QList<SimpleRecord> { { 10, "meh" } }));
	}

	void OralTest::testAutoPKeyRecordInsertSelect ()
	{
		auto adapted = PrepareRecords<AutogenPKeyRecord> (MakeDatabase ());
		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<AutogenPKeyRecord> { { 1, "0" }, { 2, "1" }, { 3, "2" } }));
	}

	void OralTest::testAutoPKeyRecordInsertRvalueReturnsPKey ()
	{
		auto adapted = Util::oral::AdaptPtr<AutogenPKeyRecord, OralFactory> (MakeDatabase ());

		QList<int> ids;
		for (int i = 0; i < 3; ++i)
			ids << adapted->Insert ({ 0, QString::number (i) });

		QCOMPARE (ids, (QList<int> { 1, 2, 3 }));
	}

	void OralTest::testAutoPKeyRecordInsertConstLvalueReturnsPKey ()
	{
		auto adapted = Util::oral::AdaptPtr<AutogenPKeyRecord, OralFactory> (MakeDatabase ());

		QList<AutogenPKeyRecord> records;
		for (int i = 0; i < 3; ++i)
			records.push_back ({ 0, QString::number (i) });

		QList<int> ids;
		for (const auto& record : records)
			ids << adapted->Insert (record);

		QCOMPARE (ids, (QList<int> { 1, 2, 3 }));
	}

	void OralTest::testAutoPKeyRecordInsertSetsPKey ()
	{
		auto adapted = Util::oral::AdaptPtr<AutogenPKeyRecord, OralFactory> (MakeDatabase ());

		QList<AutogenPKeyRecord> records;
		for (int i = 0; i < 3; ++i)
			records.push_back ({ 0, QString::number (i) });

		for (auto& record : records)
			adapted->Insert (record);

		QCOMPARE (records, (QList<AutogenPKeyRecord> { { 1, "0" }, { 2, "1" }, { 3, "2" } }));
	}

	void OralTest::testNoPKeyRecordInsertSelect ()
	{
		auto adapted = PrepareRecords<NoPKeyRecord> (MakeDatabase ());
		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<NoPKeyRecord> { { 0, "0" }, { 1, "1" }, { 2, "2" } }));
	}

	void OralTest::testNonInPlaceConstructibleRecordInsertSelect ()
	{
		auto adapted = Util::oral::AdaptPtr<NonInPlaceConstructibleRecord, OralFactory> (MakeDatabase ());
		for (int i = 0; i < 3; ++i)
			adapted->Insert ({ i, QString::number (i), 0 });

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<NonInPlaceConstructibleRecord> { { 0, "0", 0 }, { 1, "1", 0 }, { 2, "2", 0 } }));
	}

	namespace
	{
		template<typename Ex, typename F>
		void ShallThrow (F&& f)
		{
			bool failed = false;
			try
			{
				f ();
			}
			catch (const Ex&)
			{
				failed = true;
			}

			QCOMPARE (failed, true);
		}
	}

	void OralTest::testComplexConstraintsRecordInsertSelectDefault ()
	{
		auto adapted = Util::oral::AdaptPtr<ComplexConstraintsRecord, OralFactory> (MakeDatabase ());

		adapted->Insert ({ 0, "first", 1, 2 });
		ShallThrow<oral::QueryException> ([&] { adapted->Insert ({ 0, "second", 1, 2 }); });
		ShallThrow<oral::QueryException> ([&] { adapted->Insert ({ 0, "first", 1, 3 }); });
		adapted->Insert ({ 0, "second", 1, 3 });
		ShallThrow<oral::QueryException> ([&] { adapted->Insert ({ 0, "first", 1, 3 }); });

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<ComplexConstraintsRecord> { { 0, "first", 1, 2 }, { 0, "second", 1, 3 } }));
	}

	void OralTest::testComplexConstraintsRecordInsertSelectIgnore ()
	{
		auto adapted = Util::oral::AdaptPtr<ComplexConstraintsRecord, OralFactory> (MakeDatabase ());

		adapted->Insert ({ 0, "first", 1, 2 }, lco::InsertAction::Ignore);
		adapted->Insert ({ 0, "second", 1, 2 }, lco::InsertAction::Ignore);
		adapted->Insert ({ 0, "first", 1, 3 }, lco::InsertAction::Ignore);
		adapted->Insert ({ 0, "second", 1, 3 }, lco::InsertAction::Ignore);
		adapted->Insert ({ 0, "first", 1, 3 }, lco::InsertAction::Ignore);

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<ComplexConstraintsRecord> { { 0, "first", 1, 2 }, { 0, "second", 1, 3 } }));
	}

	void OralTest::testComplexConstraintsRecordInsertSelectReplace ()
	{
		auto adapted = Util::oral::AdaptPtr<ComplexConstraintsRecord, OralFactory> (MakeDatabase ());

		adapted->Insert ({ 0, "first", 1, 2 },
				lco::InsertAction::Replace::Fields<&ComplexConstraintsRecord::ID_, &ComplexConstraintsRecord::Value_>);
		adapted->Insert ({ 0, "second", 1, 2 },
				lco::InsertAction::Replace::Fields<&ComplexConstraintsRecord::Weight_, &ComplexConstraintsRecord::Age_>);
		adapted->Insert ({ 0, "first", 1, 3 },
				lco::InsertAction::Replace::Fields<&ComplexConstraintsRecord::ID_, &ComplexConstraintsRecord::Value_>);
		adapted->Insert ({ 0, "second", 1, 3 },
				lco::InsertAction::Replace::Fields<&ComplexConstraintsRecord::ID_, &ComplexConstraintsRecord::Value_>);
		adapted->Insert ({ 0, "first", 1, 3 },
				lco::InsertAction::Replace::Fields<&ComplexConstraintsRecord::ID_, &ComplexConstraintsRecord::Value_>);

		const auto& list = adapted->Select ();
		QCOMPARE (list, (QList<ComplexConstraintsRecord> { { 0, "first", 1, 3 } }));
	}

	void OralTest::benchSimpleRecordAdapt ()
	{
		if constexpr (OralBench)
		{
			auto db = MakeDatabase ();
			Util::oral::Adapt<SimpleRecord, OralFactory> (db);

			QBENCHMARK { Util::oral::Adapt<SimpleRecord> (db); }
		}
	}

	void OralTest::benchBaselineInsert ()
	{
		if constexpr (OralBench)
		{
			auto db = MakeDatabase ();
			Util::oral::Adapt<SimpleRecord, OralFactory> (db);

			QSqlQuery query { db };
			query.prepare ("INSERT OR IGNORE INTO SimpleRecord (ID, Value) VALUES (:id, :val);");

			QBENCHMARK
			{
				query.bindValue (":id", 0);
				query.bindValue (":val", "0");
				query.exec ();
			}
		}
	}

	void OralTest::benchSimpleRecordInsert ()
	{
		if constexpr (OralBench)
		{
			auto db = MakeDatabase ();
			const auto& adapted = Util::oral::Adapt<SimpleRecord, OralFactory> (db);

			QBENCHMARK { adapted.Insert ({ 0, "0" }, lco::InsertAction::Ignore); }
		}
	}

	void OralTest::benchBaselineUpdate ()
	{
		if constexpr (OralBench)
		{
			auto db = MakeDatabase ();
			const auto& adapted = Util::oral::Adapt<SimpleRecord, OralFactory> (db);
			adapted.Insert ({ 0, "0" });

			QSqlQuery query { db };
			query.prepare ("UPDATE SimpleRecord SET Value = :val WHERE Id = :id;");

			QBENCHMARK
			{
				query.bindValue (":id", 0);
				query.bindValue (":val", "1");
				query.exec ();
			}
		}
	}

	void OralTest::benchSimpleRecordUpdate ()
	{
		if constexpr (OralBench)
		{
			auto db = MakeDatabase ();
			auto adapted = Util::oral::Adapt<SimpleRecord, OralFactory> (db);
			adapted.Insert ({ 0, "0" });

			QBENCHMARK { adapted.Update ({ 0, "1" }); }
		}
	}
}
}
