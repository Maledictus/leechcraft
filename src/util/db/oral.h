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

#include <stdexcept>
#include <type_traits>
#include <memory>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/filter_if.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/zip.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/variant/variant.hpp>
#include <boost/optional.hpp>
#include <QStringList>
#include <QDateTime>
#include <QPair>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDateTime>
#include <QtDebug>
#include <util/sll/qtutil.h>
#include <util/sll/prelude.h>
#include <util/sll/typelist.h>
#include <util/sll/typegetter.h>
#include <util/sll/unreachable.h>
#include <util/db/dblock.h>
#include <util/db/util.h>
#include "oraltypes.h"

using QSqlQuery_ptr = std::shared_ptr<QSqlQuery>;

namespace LeechCraft
{
namespace Util
{
namespace oral
{
	class QueryException : public std::runtime_error
	{
		const QSqlQuery_ptr Query_;
	public:
		QueryException (const std::string& str, const QSqlQuery_ptr& q)
		: std::runtime_error (str)
		, Query_ (q)
		{
		}

		virtual ~QueryException () throw ()
		{
		}

		const QSqlQuery_ptr& GetQueryPtr () const
		{
			return Query_;
		}

		const QSqlQuery& GetQuery () const
		{
			return *Query_;
		}
	};

	namespace detail
	{
		template<typename U>
		using MorpherDetector = decltype (std::declval<U> ().FieldNameMorpher (QString {}));

		template<typename T>
		QString MorphFieldName (const QString& str)
		{
			if constexpr (IsDetected_v<MorpherDetector, T>)
				return T::FieldNameMorpher (str);
			else
				return str;
		}

		template<typename Seq, int Idx>
		struct GetFieldName
		{
			static QString value ()
			{
				return MorphFieldName<Seq> (boost::fusion::extension::struct_member_name<Seq, Idx>::call ());
			}
		};

		template<typename S>
		constexpr auto SeqIndices = std::make_index_sequence<boost::fusion::result_of::size<S>::type::value> {};

		template<typename S>
		struct GetFieldsNames
		{
			QStringList operator() () const
			{
				return Run (SeqIndices<S>);
			}
		private:
			template<size_t... Vals>
			QStringList Run (std::index_sequence<Vals...>) const
			{
				return { GetFieldName<S, Vals>::value ()... };
			}
		};

		template<typename Seq, int Idx>
		struct GetBoundName
		{
			static QString value () { return ':' + Seq::ClassName () + "_" + GetFieldName<Seq, Idx>::value (); }
		};
	}

	template<typename T, typename = void>
	struct Type2Name
	{
		QString operator() () const
		{
			if constexpr (HasType<T> (Typelist<int, qulonglong, bool> {}) || std::is_enum_v<T>)
				return "INTEGER";
			else if constexpr (std::is_same_v<T, double>)
				return "REAL";
			else if constexpr (std::is_same_v<T, QString> || std::is_same_v<T, QDateTime>)
				return "TEXT";
			else if constexpr (std::is_same_v<T, QByteArray>)
				return "BLOB";
			else
				static_assert (std::is_same_v<T, struct Dummy>, "Unsupported type");
		}
	};

	template<typename T>
	struct Type2Name<Unique<T>>
	{
		QString operator() () const { return Type2Name<T> () () + " UNIQUE"; }
	};

	template<typename T, typename... Tags>
	struct Type2Name<PKey<T, Tags...>>
	{
		QString operator() () const { return Type2Name<T> () () + " PRIMARY KEY"; }
	};

	template<typename... Tags>
	struct Type2Name<PKey<int, Tags...>>
	{
		QString operator() () const { return Type2Name<int> () () + " PRIMARY KEY AUTOINCREMENT"; }
	};

	template<typename Seq, int Idx>
	struct Type2Name<References<Seq, Idx>>
	{
		QString operator() () const
		{
			return Type2Name<ReferencesValue_t<Seq, Idx>> () () +
					" REFERENCES " + Seq::ClassName () + " (" + detail::GetFieldName<Seq, Idx>::value () + ") ON DELETE CASCADE";
		}
	};

	template<typename T, typename = void>
	struct ToVariant
	{
		QVariant operator() (const T& t) const
		{
			return t;
		}
	};

	template<>
	struct ToVariant<QDateTime>
	{
		QVariant operator() (const QDateTime& t) const
		{
			return t.toString (Qt::ISODate);
		}
	};

	template<typename E>
	struct ToVariant<E, std::enable_if_t<std::is_enum<E>::value>>
	{
		QVariant operator() (E e) const
		{
			return static_cast<qint64> (e);
		}
	};

	template<typename T>
	struct ToVariant<Unique<T>>
	{
		QVariant operator() (const Unique<T>& t) const
		{
			return static_cast<UniqueValue_t<T>> (t);
		}
	};

	template<typename T, typename... Tags>
	struct ToVariant<PKey<T, Tags...>>
	{
		QVariant operator() (const PKey<T, Tags...>& t) const
		{
			return static_cast<PKeyValue_t<T, Tags...>> (t);
		}
	};

	template<typename Seq, int Idx>
	struct ToVariant<References<Seq, Idx>>
	{
		QVariant operator() (const References<Seq, Idx>& t) const
		{
			return static_cast<ReferencesValue_t<Seq, Idx>> (t);
		}
	};

	template<typename T, typename = void>
	struct FromVariant
	{
		T operator() (const QVariant& var) const
		{
			return var.value<T> ();
		}
	};

	template<>
	struct FromVariant<QDateTime>
	{
		QDateTime operator() (const QVariant& var) const
		{
			return QDateTime::fromString (var.toString (), Qt::ISODate);
		}
	};

	template<typename E>
	struct FromVariant<E, std::enable_if_t<std::is_enum<E>::value>>
	{
		E operator() (const QVariant& var) const
		{
			return static_cast<E> (var.value<qint64> ());
		}
	};

	template<typename T>
	struct FromVariant<Unique<T>>
	{
		T operator() (const QVariant& var) const
		{
			return var.value<T> ();
		}
	};

	template<typename T, typename... Tags>
	struct FromVariant<PKey<T, Tags...>>
	{
		T operator() (const QVariant& var) const
		{
			return var.value<T> ();
		}
	};

	template<typename Seq, int Idx>
	struct FromVariant<References<Seq, Idx>>
	{
		auto operator() (const QVariant& var) const
		{
			return var.value<ReferencesValue_t<Seq, Idx>> ();
		}
	};

	enum class InsertAction
	{
		Default,
		Ignore,
		Replace
	};

	constexpr size_t InsertActionCount = 3;

	namespace detail
	{
		template<typename T>
		QVariant ToVariantF (const T& t)
		{
			return ToVariant<T> {} (t);
		}

		struct CachedFieldsData
		{
			QString Table_;
			QSqlDatabase DB_;

			QList<QString> Fields_;
			QList<QString> BoundFields_;
		};

		template<typename T>
		auto MakeInserter (const CachedFieldsData& data, const QSqlQuery_ptr& insertQuery, bool bindPrimaryKey)
		{
			return [data, insertQuery, bindPrimaryKey] (const T& t)
			{
				boost::fusion::fold (t, data.BoundFields_.begin (),
						[&] (auto pos, const auto& elem)
						{
							using Elem = std::decay_t<decltype (elem)>;
							if (bindPrimaryKey || !IsPKey<Elem>::value)
								insertQuery->bindValue (*pos++, ToVariantF (elem));
							return pos;
						});

				if (!insertQuery->exec ())
				{
					DBLock::DumpError (*insertQuery);
					throw QueryException ("insert query execution failed", insertQuery);
				}
			};
		}

		template<typename T>
		struct Lazy
		{
			using type = T;
		};

		template<typename Seq, int Idx>
		using ValueAtC_t = typename boost::fusion::result_of::value_at_c<Seq, Idx>::type;

		template<typename Seq, typename Idx>
		using ValueAt_t = typename boost::fusion::result_of::value_at<Seq, Idx>::type;

		template<typename Seq, typename MemberIdx = boost::mpl::int_<0>>
		struct FindPKey
		{
			static_assert ((boost::fusion::result_of::size<Seq>::value) != (MemberIdx::value),
					"Primary key not found");

			using result_type = typename std::conditional_t<
						IsPKey<ValueAt_t<Seq, MemberIdx>>::value,
						Lazy<MemberIdx>,
						Lazy<FindPKey<Seq, typename boost::mpl::next<MemberIdx>::type>>
					>::type;
		};

		template<typename Seq>
		using FindPKeyDetector = boost::mpl::int_<FindPKey<Seq>::result_type::value>;

		template<typename Seq>
		constexpr auto HasPKey = IsDetected_v<FindPKeyDetector, Seq>;

		template<typename Seq>
		constexpr auto HasAutogenPKey ()
		{
			if constexpr (HasPKey<Seq>)
				return !HasType<NoAutogen> (AsTypelist_t<ValueAtC_t<Seq, FindPKey<Seq>::result_type::value>> {});
			else
				return false;
		}

		inline QString GetInsertPrefix (InsertAction action)
		{
			switch (action)
			{
			case InsertAction::Default:
				return "INSERT";
			case InsertAction::Ignore:
				return "INSERT OR IGNORE";
			case InsertAction::Replace:
				return "INSERT OR REPLACE";
			}

			Util::Unreachable ();
		}

		template<typename Seq>
		struct AdaptInsert
		{
			const CachedFieldsData Data_;
			const QString InsertSuffix_;

			constexpr static bool HasAutogen_ = HasAutogenPKey<Seq> ();

			mutable std::array<QSqlQuery_ptr, InsertActionCount> Queries_;
		public:
			AdaptInsert (CachedFieldsData data)
			: Data_
			{
				[data] () mutable
				{
					if constexpr (HasAutogen_)
					{
						constexpr auto index = FindPKey<Seq>::result_type::value;
						data.Fields_.removeAt (index);
						data.BoundFields_.removeAt (index);
					}
					return data;
				} ()
			}
			, InsertSuffix_ { " INTO " + Data_.Table_ +
					" (" + QStringList { Data_.Fields_ }.join (", ") + ") VALUES (" +
					QStringList { Data_.BoundFields_ }.join (", ") + ");" }
			{
			}

			auto operator() (Seq& t, InsertAction action = InsertAction::Default) const
			{
				return Run<true> (t, action);
			}

			auto operator() (const Seq& t, InsertAction action = InsertAction::Default) const
			{
				return Run<false> (t, action);
			}
		private:
			template<bool UpdatePKey, typename Val>
			auto Run (Val&& t, InsertAction action) const
			{
				const auto query = GetQuery (action);

				MakeInserter<Seq> (Data_, query, !HasAutogen_) (t);

				if constexpr (HasAutogen_)
				{
					constexpr auto index = FindPKey<Seq>::result_type::value;

					const auto& lastId = FromVariant<ValueAtC_t<Seq, index>> {} (query->lastInsertId ());
					if constexpr (UpdatePKey)
						boost::fusion::at_c<index> (t) = lastId;
					else
						return lastId;
				}
			}

			auto GetQuery (InsertAction action) const
			{
				auto& query = Queries_ [static_cast<size_t> (action)];
				if (!query)
				{
					query = std::make_shared<QSqlQuery> (Data_.DB_);
					query->prepare (GetInsertPrefix (action) + InsertSuffix_);
				}
				return query;
			}
		};

		template<typename Seq, bool HasPKey = HasPKey<Seq>>
		class AdaptUpdate
		{
			std::function<void (Seq)> Updater_;
		public:
			AdaptUpdate (const CachedFieldsData& data)
			{
				if constexpr (HasPKey)
				{
					const auto index = FindPKey<Seq>::result_type::value;

					auto removedFields = data.Fields_;
					auto removedBoundFields = data.BoundFields_;

					const auto& fieldName = removedFields.takeAt (index);
					const auto& boundName = removedBoundFields.takeAt (index);

					const auto& statements = Util::ZipWith (removedFields, removedBoundFields,
							[] (const QString& s1, const QString& s2) -> QString
								{ return s1 + " = " + s2; });

					const auto& update = "UPDATE " + data.Table_ +
							" SET " + QStringList { statements }.join (", ") +
							" WHERE " + fieldName + " = " + boundName + ";";

					const auto updateQuery = std::make_shared<QSqlQuery> (data.DB_);
					updateQuery->prepare (update);
					Updater_ = MakeInserter<Seq> (data, updateQuery, true);
				}
			}

			template<bool B = HasPKey>
			std::enable_if_t<B> operator() (const Seq& seq)
			{
				Updater_ (seq);
			}
		};

		template<typename Seq, bool HasPKey = HasPKey<Seq>>
		struct AdaptDelete
		{
			std::function<void (Seq)> Deleter_;
		public:
			template<bool B = HasPKey>
			AdaptDelete (const CachedFieldsData& data, std::enable_if_t<B>* = nullptr)
			{
				const auto index = FindPKey<Seq>::result_type::value;

				const auto& boundName = data.BoundFields_.at (index);
				const auto& del = "DELETE FROM " + data.Table_ +
						" WHERE " + data.Fields_.at (index) + " = " + boundName + ";";

				const auto deleteQuery = std::make_shared<QSqlQuery> (data.DB_);
				deleteQuery->prepare (del);

				Deleter_ = [deleteQuery, boundName] (const Seq& t)
				{
					constexpr auto index = FindPKey<Seq>::result_type::value;
					deleteQuery->bindValue (boundName, ToVariantF (boost::fusion::at_c<index> (t)));
					if (!deleteQuery->exec ())
						throw QueryException ("delete query execution failed", deleteQuery);
				};
			}

			template<bool B = HasPKey>
			AdaptDelete (const CachedFieldsData&, std::enable_if_t<!B>* = nullptr)
			{
			}

			template<bool B = HasPKey>
			std::enable_if_t<B> operator() (const Seq& seq)
			{
				Deleter_ (seq);
			}
		};

		template<typename T, typename... Args>
		using AggregateDetector_t = decltype (new T { std::declval<Args> ()... });

		template<typename T, size_t... Indices>
		T InitializeFromQuery (const QSqlQuery_ptr& q, std::index_sequence<Indices...>)
		{
			if constexpr (IsDetected_v<AggregateDetector_t, T, ValueAtC_t<T, Indices>...>)
				return T { FromVariant<ValueAtC_t<T, Indices>> {} (q->value (Indices))... };
			else
			{
				T t;
				const auto dummy = std::initializer_list<int>
				{
					(static_cast<void> (boost::fusion::at_c<Indices> (t) = FromVariant<ValueAtC_t<T, Indices>> {} (q->value (Indices))), 0)...
				};
				Q_UNUSED (dummy);
				return t;
			}
		}

		template<typename T>
		QList<T> PerformSelect (QSqlQuery_ptr q)
		{
			if (!q->exec ())
				throw QueryException ("fetch query execution failed", q);

			QList<T> result;
			while (q->next ())
				result << InitializeFromQuery<T> (q, SeqIndices<T>);
			q->finish ();
			return result;
		}

		template<typename T>
		std::function<QList<T> ()> AdaptSelectAll (const CachedFieldsData& data)
		{
			const auto& selectAll = "SELECT " + QStringList { data.Fields_ }.join (", ") + " FROM " + data.Table_ + ";";
			const auto selectQuery = std::make_shared<QSqlQuery> (data.DB_);
			selectQuery->prepare (selectAll);
			return [selectQuery] { return PerformSelect<T> (selectQuery); };
		}

		template<int HeadT, int... TailT>
		struct FieldsUnpacker
		{
			static const int Head = HeadT;
			using Tail_t = FieldsUnpacker<TailT...>;
		};

		template<int HeadT>
		struct FieldsUnpacker<HeadT>
		{
			static const int Head = HeadT;
			using Tail_t = std::false_type;
		};

		template<typename FieldsUnpacker, typename HeadArg, typename... TailArgs>
		struct ValueBinder
		{
			QSqlQuery_ptr Query_;
			QList<QString> BoundFields_;

			void operator() (const HeadArg& arg, const TailArgs&... tail) const
			{
				Query_->bindValue (BoundFields_.at (FieldsUnpacker::Head), arg);

				ValueBinder<typename FieldsUnpacker::Tail_t, TailArgs...> { Query_, BoundFields_ } (tail...);
			}
		};

		template<typename FieldsUnpacker, typename HeadArg>
		struct ValueBinder<FieldsUnpacker, HeadArg>
		{
			QSqlQuery_ptr Query_;
			QList<QString> BoundFields_;

			void operator() (const HeadArg& arg) const
			{
				Query_->bindValue (BoundFields_.at (FieldsUnpacker::Head), arg);
			}
		};

		enum class ExprType
		{
			LeafStaticPlaceholder,
			LeafData,

			Greater,
			Less,
			Equal,
			Geq,
			Leq,
			Neq,

			And,
			Or
		};

		inline QString TypeToSql (ExprType type)
		{
			switch (type)
			{
			case ExprType::Greater:
				return ">";
			case ExprType::Less:
				return "<";
			case ExprType::Equal:
				return "=";
			case ExprType::Geq:
				return ">=";
			case ExprType::Leq:
				return "<=";
			case ExprType::Neq:
				return "!=";
			case ExprType::And:
				return "AND";
			case ExprType::Or:
				return "OR";

			case ExprType::LeafStaticPlaceholder:
			case ExprType::LeafData:
				return "invalid type";
			}

			Util::Unreachable ();
		}

		constexpr bool IsCompatible (ExprType type)
		{
			return type == ExprType::And ||
					type == ExprType::Or ||
					type == ExprType::LeafStaticPlaceholder ||
					type == ExprType::LeafData;
		}

		constexpr bool CheckCompatible (ExprType t1, ExprType t2)
		{
			return IsCompatible (t1) || IsCompatible (t2);
		}

		constexpr bool IsRelational (ExprType type)
		{
			return type == ExprType::Greater ||
					type == ExprType::Less ||
					type == ExprType::Equal ||
					type == ExprType::Geq ||
					type == ExprType::Leq ||
					type == ExprType::Neq;
		}

		template<typename T>
		struct ToSqlState
		{
			int LastID_;
			QVariantMap BoundMembers_;
		};

		template<typename Seq, typename L, typename R>
		using ComparableDetector = decltype (std::declval<typename L::template ValueType_t<Seq>> () == std::declval<typename R::template ValueType_t<Seq>> ());

		template<typename Seq, typename L, typename R>
		constexpr auto AreComparableTypes = IsDetected_v<ComparableDetector, Seq, L, R> || IsDetected_v<ComparableDetector, Seq, R, L>;

		template<ExprType Type, typename Seq, typename L, typename R, typename = void>
		struct RelationalTypesChecker : std::true_type {};

		template<typename Seq, typename L, typename R, typename = void>
		struct RelationalTypesCheckerBase : std::false_type {};

		template<typename Seq, typename L, typename R>
		struct RelationalTypesCheckerBase<Seq, L, R, std::enable_if_t<AreComparableTypes<Seq, L, R>>> : std::true_type {};

		template<ExprType Type, typename Seq, typename L, typename R>
		struct RelationalTypesChecker<Type, Seq, L, R, std::enable_if_t<IsRelational (Type)>> : RelationalTypesCheckerBase<Seq, L, R> {};

		template<ExprType Type, typename L = void, typename R = void>
		class ExprTree
		{
			L Left_;
			R Right_;
		public:
			ExprTree (const L& l, const R& r)
			: Left_ (l)
			, Right_ (r)
			{
			}

			template<typename T>
			QString ToSql (ToSqlState<T>& state) const
			{
				static_assert (RelationalTypesChecker<Type, T, L, R>::value,
						"Incompatible types passed to a relational operator.");

				return Left_.ToSql (state) + " " + TypeToSql (Type) + " " + Right_.ToSql (state);
			}
		};

		template<int Idx>
		class ExprTree<ExprType::LeafStaticPlaceholder, boost::mpl::int_<Idx>, void>
		{
		public:
			template<typename T>
			using ValueType_t = ValueAtC_t<T, Idx>;

			template<typename T>
			QString ToSql (ToSqlState<T>&) const
			{
				static_assert (Idx < boost::fusion::result_of::size<T>::type::value, "Index out of bounds.");
				return detail::GetFieldsNames<T> {} ().at (Idx);
			}
		};

		template<typename T>
		class ExprTree<ExprType::LeafData, T, void>
		{
			T Data_;
		public:
			template<typename>
			using ValueType_t = T;

			ExprTree (const T& t)
			: Data_ (t)
			{
			}

			template<typename ObjT>
			QString ToSql (ToSqlState<ObjT>& state) const
			{
				const auto& name = ":bound_" + QString::number (++state.LastID_);
				state.BoundMembers_ [name] = ToVariantF (Data_);
				return name;
			}
		};

		template<ExprType LType, typename LL, typename LR, ExprType RType, typename RL, typename RR>
		ExprTree<ExprType::Less, ExprTree<LType, LL, LR>, ExprTree<RType, RL, RR>> operator< (const ExprTree<LType, LL, LR>& left, const ExprTree<RType, RL, RR>& right)
		{
			static_assert (CheckCompatible (LType, RType), "comparing incompatible subexpressions");
			return { left, right };
		}

		template<ExprType LType, typename LL, typename LR, typename R>
		ExprTree<ExprType::Less, ExprTree<LType, LL, LR>, ExprTree<ExprType::LeafData, R>> operator< (const ExprTree<LType, LL, LR>& left, const R& right)
		{
			return left < ExprTree<ExprType::LeafData, R> { right };
		}

		template<ExprType RType, typename RL, typename RR, typename L>
		ExprTree<ExprType::Less, ExprTree<ExprType::LeafData, L>, ExprTree<RType, RL, RR>> operator< (const L& left, const ExprTree<RType, RL, RR>& right)
		{
			return ExprTree<ExprType::LeafData, L> { left } < right;
		}

		template<ExprType LType, typename LL, typename LR, ExprType RType, typename RL, typename RR>
		ExprTree<ExprType::Equal, ExprTree<LType, LL, LR>, ExprTree<RType, RL, RR>> operator== (const ExprTree<LType, LL, LR>& left, const ExprTree<RType, RL, RR>& right)
		{
			static_assert (CheckCompatible (LType, RType), "comparing incompatible subexpressions");
			return { left, right };
		}

		template<ExprType LType, typename LL, typename LR, typename R>
		ExprTree<ExprType::Equal, ExprTree<LType, LL, LR>, ExprTree<ExprType::LeafData, R>> operator== (const ExprTree<LType, LL, LR>& left, const R& right)
		{
			return left == ExprTree<ExprType::LeafData, R> { right };
		}

		template<ExprType RType, typename RL, typename RR, typename L>
		ExprTree<ExprType::Equal, ExprTree<ExprType::LeafData, L>, ExprTree<RType, RL, RR>> operator== (const L& left, const ExprTree<RType, RL, RR>& right)
		{
			return ExprTree<ExprType::LeafData, L> { left } == right;
		}

		template<ExprType LType, typename LL, typename LR, ExprType RType, typename RL, typename RR>
		ExprTree<ExprType::And, ExprTree<LType, LL, LR>, ExprTree<RType, RL, RR>> operator&& (const ExprTree<LType, LL, LR>& left, const ExprTree<RType, RL, RR>& right)
		{
			return { left, right };
		}

		template<ExprType LType, typename LL, typename LR, typename R>
		ExprTree<ExprType::And, ExprTree<LType, LL, LR>, ExprTree<ExprType::LeafData, R>> operator&& (const ExprTree<LType, LL, LR>& left, const R& right)
		{
			return left && ExprTree<ExprType::LeafData, R> { right };
		}

		template<ExprType RType, typename RL, typename RR, typename L>
		ExprTree<ExprType::And, ExprTree<ExprType::LeafData, L>, ExprTree<RType, RL, RR>> operator&& (const L& left, const ExprTree<RType, RL, RR>& right)
		{
			return ExprTree<ExprType::LeafData, L> { left } && right;
		}

		template<typename Seq, ExprType Type, typename L, typename R>
		QPair<QString, std::function<void (QSqlQuery_ptr)>> HandleExprTree (const ExprTree<Type, L, R>& tree)
		{
			ToSqlState<Seq> state { 0, {} };

			const auto& sql = tree.ToSql (state);

			return
			{
				sql,
				[state] (const QSqlQuery_ptr& query)
				{
					for (const auto& pair : Stlize (state.BoundMembers_))
						query->bindValue (pair.first, pair.second);
				}
			};
		}
	}

	namespace sph
	{
		template<int Idx>
		using pos = detail::ExprTree<detail::ExprType::LeafStaticPlaceholder, boost::mpl::int_<Idx>>;

		static constexpr pos<0> _0 = {};
		static constexpr pos<1> _1 = {};
		static constexpr pos<2> _2 = {};
		static constexpr pos<3> _3 = {};
		static constexpr pos<4> _4 = {};

		template<int Idx>
		static constexpr pos<Idx> _ = {};
	};

	namespace detail
	{
		template<typename T>
		class SelectByFieldsWrapper
		{
			const CachedFieldsData Cached_;
		public:
			SelectByFieldsWrapper (const CachedFieldsData& data)
			: Cached_ (data)
			{
			}

			template<ExprType Type, typename L, typename R>
			QList<T> operator() (const ExprTree<Type, L, R>& tree) const
			{
				const auto& treeResult = HandleExprTree<T> (tree);

				const auto& selectAll = "SELECT " + QStringList { Cached_.Fields_ }.join (", ") +
						" FROM " + Cached_.Table_ +
						" WHERE " + treeResult.first + ";";

				const auto query = std::make_shared<QSqlQuery> (Cached_.DB_);
				query->prepare (selectAll);
				treeResult.second (query);
				return PerformSelect<T> (query);
			}

			template<int Idx, ExprType Type, typename L, typename R>
			QList<ValueAtC_t<T, Idx>> operator() (sph::pos<Idx>, const ExprTree<Type, L, R>& tree) const
			{
				const auto& treeResult = HandleExprTree<T> (tree);

				const auto& selectOne = "SELECT " + Cached_.Fields_.value (Idx) +
						" FROM " + Cached_.Table_ +
						" WHERE " + treeResult.first + ";";

				const auto query = std::make_shared<QSqlQuery> (Cached_.DB_);
				query->prepare (selectOne);
				treeResult.second (query);

				if (!query->exec ())
					throw QueryException ("fetch query execution failed", query);

				using Type_t = ValueAtC_t<T, Idx>;

				QList<Type_t> result;
				while (query->next ())
					result << FromVariant<Type_t> {} (query->value (0));
				query->finish ();
				return result;
			}
		};

		template<typename T>
		class SelectOneByFieldsWrapper
		{
			const SelectByFieldsWrapper<T> Select_;
		public:
			SelectOneByFieldsWrapper (const CachedFieldsData& data)
			: Select_ { data }
			{
			}

			template<ExprType Type, typename L, typename R>
			boost::optional<T> operator() (const ExprTree<Type, L, R>& tree) const
			{
				const auto& result = Select_ (tree);
				if (result.isEmpty ())
					return {};

				return result.value (0);
			}

			template<int Idx, ExprType Type, typename L, typename R>
			boost::optional<ValueAtC_t<T, Idx>> operator() (sph::pos<Idx> p, const ExprTree<Type, L, R>& tree) const
			{
				const auto& result = Select_ (p, tree);
				if (result.isEmpty ())
					return {};

				return result.value (0);
			}
		};

		template<typename T>
		class DeleteByFieldsWrapper
		{
			const CachedFieldsData Cached_;
		public:
			DeleteByFieldsWrapper (const CachedFieldsData& data)
			: Cached_ (data)
			{
			}

			template<ExprType Type, typename L, typename R>
			void operator() (const ExprTree<Type, L, R>& tree) const
			{
				const auto& treeResult = HandleExprTree<T> (tree);

				const auto& selectAll = "DELETE FROM " + Cached_.Table_ +
						" WHERE " + treeResult.first + ";";

				const auto query = std::make_shared<QSqlQuery> (Cached_.DB_);
				query->prepare (selectAll);
				treeResult.second (query);
				query->exec ();
			}
		};

		template<typename T>
		SelectByFieldsWrapper<T> AdaptSelectFields (const CachedFieldsData& data)
		{
			return { data };
		}

		template<typename T>
		SelectOneByFieldsWrapper<T> AdaptSelectOneFields (const CachedFieldsData& data)
		{
			return { data };
		}

		template<typename T>
		DeleteByFieldsWrapper<T> AdaptDeleteFields (const CachedFieldsData& data)
		{
			return { data };
		}

		template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename MemberIdx>
		struct FieldInfo
		{
		};

		template<typename To, typename OrigSeq, typename OrigIdx, typename T>
		struct FieldAppender
		{
			using value_type = To;
		};

		template<typename To, typename OrigSeq, typename OrigIdx, typename RefSeq, int RefIdx>
		struct FieldAppender<To, OrigSeq, OrigIdx, References<RefSeq, RefIdx>>
		{
			using value_type = typename boost::fusion::result_of::as_vector<
					typename boost::fusion::result_of::push_front<
						To,
						FieldInfo<OrigSeq, OrigIdx, RefSeq, boost::mpl::int_<RefIdx>>
					>::type
				>::type;
		};

		template<typename Seq, typename MemberIdx>
		struct CollectRefs_
		{
			using type_list = typename FieldAppender<
					typename CollectRefs_<Seq, typename boost::mpl::next<MemberIdx>::type>::type_list,
					Seq,
					MemberIdx,
					typename std::decay<typename boost::fusion::result_of::at<Seq, MemberIdx>::type>::type
				>::value_type;
		};

		template<typename Seq>
		struct CollectRefs_<Seq, typename boost::fusion::result_of::size<Seq>::type>
		{
			using type_list = boost::fusion::vector<>;
		};

		template<typename Seq>
		struct CollectRefs : CollectRefs_<Seq, boost::mpl::int_<0>>
		{
		};

		struct Ref2Select
		{
			template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename RefIdx>
			QStringList operator() (const QStringList& init, const FieldInfo<OrigSeq, OrigIdx, RefSeq, RefIdx>&) const
			{
				const auto& thisQualified = OrigSeq::ClassName () + "." + GetFieldName<OrigSeq, OrigIdx::value>::value ();
				return init + QStringList { thisQualified + " = " + GetBoundName<RefSeq, RefIdx::value>::value () };
			}
		};

		template<typename T>
		struct ExtrObj;

		template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename MemberIdx>
		struct ExtrObj<FieldInfo<OrigSeq, OrigIdx, RefSeq, MemberIdx>>
		{
			using type = RefSeq;
		};

		struct SingleBind
		{
			QSqlQuery_ptr Q_;

			template<typename ObjType, typename OrigSeq, typename OrigIdx, typename RefSeq, typename RefIdx>
			void operator() (const boost::fusion::vector2<ObjType, const FieldInfo<OrigSeq, OrigIdx, RefSeq, RefIdx>&>& pair) const
			{
				Q_->bindValue (GetBoundName<RefSeq, RefIdx::value>::value (),
						ToVariantF (boost::fusion::at<RefIdx> (boost::fusion::at_c<0> (pair))));
			}
		};

		template<typename T, typename RefSeq>
		struct MakeBinder
		{
			using transform_view = typename boost::mpl::transform<RefSeq, ExtrObj<boost::mpl::_1>>;
			using objects_view = typename transform_view::type;
			using objects_vector = typename boost::fusion::result_of::as_vector<objects_view>::type;

			QSqlQuery_ptr Q_;

			QList<T> operator() (const objects_vector& objs)
			{
				boost::fusion::for_each (boost::fusion::zip (objs, RefSeq {}), SingleBind { Q_ });
				return PerformSelect<T> (Q_);
			}
		};

		template<typename T, typename Ret>
		struct WrapAsFunc
		{
			using type = std::function<QList<Ret> (T)>;
		};

		template<typename T, typename Ret>
		using WrapAsFunc_t = typename WrapAsFunc<T, Ret>::type;

		template<typename T>
		struct MakeSingleBinder
		{
			const CachedFieldsData Data_;

			template<typename Vec, typename OrigObj, typename OrigIdx, typename RefObj, typename RefIdx>
			auto operator() (Vec vec, const FieldInfo<OrigObj, OrigIdx, RefObj, RefIdx>&)
			{
				const auto& boundName = GetBoundName<OrigObj, OrigIdx::value>::value ();
				const auto& query = "SELECT " + QStringList { Data_.Fields_ }.join (", ") +
						" FROM " + Data_.Table_ +
						" WHERE " + GetFieldName<OrigObj, OrigIdx::value>::value () + " = " + boundName +
						";";
				const auto selectQuery = std::make_shared<QSqlQuery> (Data_.DB_);
				selectQuery->prepare (query);

				auto inserter = [selectQuery, boundName] (const RefObj& obj)
				{
					selectQuery->bindValue (boundName, ToVariantF (boost::fusion::at<RefIdx> (obj)));
					return PerformSelect<T> (selectQuery);
				};

				return boost::fusion::push_back (vec, WrapAsFunc_t<RefObj, T> { inserter });
			}
		};


		template<typename T, typename ObjInfo>
		void AdaptSelectRef (const CachedFieldsData& data, ObjInfo& info)
		{
			constexpr auto refsCount = CollectRefs<T>::type_list::size::value;
			if constexpr (refsCount > 0)
			{
				using references_list = typename CollectRefs<T>::type_list;
				const auto& statements = boost::fusion::fold (references_list {}, QStringList {}, Ref2Select {});

				const auto& selectAll = "SELECT " + QStringList { data.Fields_ }.join (", ") +
						" FROM " + data.Table_ +
						(statements.isEmpty () ? "" : " WHERE ") + statements.join (" AND ") +
						";";
				const auto selectQuery = std::make_shared<QSqlQuery> (data.DB_);
				selectQuery->prepare (selectAll);

				info.SelectByFKeysActor_ = MakeBinder<T, references_list> { selectQuery };

				if constexpr (refsCount > 1)
				{
					auto singleSelectors = boost::fusion::fold (references_list {},
							boost::fusion::vector<> {},
							MakeSingleBinder<T> { data });
					info.SingleFKeySelectors_ = boost::fusion::as_vector (singleSelectors);
				}
			}
		}

		template<typename T>
		using ConstraintsDetector = typename T::Constraints;

		template<typename T>
		using ConstraintsType = Util::IsDetected_t<Constraints<>, ConstraintsDetector, T>;

		template<typename T>
		struct ConstraintToString;

		template<int... Fields>
		struct ConstraintToString<UniqueSubset<Fields...>>
		{
			QString operator() (const CachedFieldsData& data) const
			{
				return "UNIQUE (" + QStringList { data.Fields_.value (Fields)... }.join (", ") + ")";
			}
		};

		template<int... Fields>
		struct ConstraintToString<PrimaryKey<Fields...>>
		{
			QString operator() (const CachedFieldsData& data) const
			{
				return "PRIMARY KEY (" + QStringList { data.Fields_.value (Fields)... }.join (", ") + ")";
			}
		};

		template<typename... Args>
		QStringList GetConstraintsStringList (Constraints<Args...>, const CachedFieldsData& data)
		{
			return QStringList { ConstraintToString<Args> {} (data)... };
		}

		template<typename T, size_t... Indices>
		QList<QString> GetTypes (std::index_sequence<Indices...>)
		{
			return { Type2Name<ValueAtC_t<T, Indices>> {} ()... };
		}

		template<typename T>
		QString AdaptCreateTable (const CachedFieldsData& data)
		{
			const auto& types = GetTypes<T> (SeqIndices<T>);

			const auto& constraints = GetConstraintsStringList (ConstraintsType<T> {}, data);
			const auto& constraintsStr = constraints.isEmpty () ?
					QString {} :
					(", " + constraints.join (", "));

			const auto& statements = Util::ZipWith (types, data.Fields_,
					[] (const QString& type, const QString& field) { return field + " " + type; });
			return "CREATE TABLE " +
					data.Table_ +
					" (" +
					statements.join (", ") +
					constraintsStr +
					");";
		}

		template<typename T, typename Enable = void>
		struct ObjectInfoFKeysHelper
		{
		};

		template<typename T>
		struct ObjectInfoFKeysHelper<T, typename std::enable_if<CollectRefs<T>::type_list::size::value == 1, void>::type>
		{
			std::function<QList<T> (typename MakeBinder<T, typename CollectRefs<T>::type_list>::objects_vector)> SelectByFKeysActor_;
		};

		template<typename T>
		struct ObjectInfoFKeysHelper<T, typename std::enable_if<CollectRefs<T>::type_list::size::value >= 2, void>::type>
		{
			using objects_vector = typename MakeBinder<T, typename CollectRefs<T>::type_list>::objects_vector;
			std::function<QList<T> (objects_vector)> SelectByFKeysActor_;

			using transform_view = typename boost::mpl::transform<objects_vector, WrapAsFunc<boost::mpl::_1, T>>::type;
			typename boost::fusion::result_of::as_vector<transform_view>::type SingleFKeySelectors_;
		};

		template<typename T>
		CachedFieldsData BuildCachedFieldsData (const QSqlDatabase& db, const QString& table = T::ClassName ())
		{
			const auto& fields = detail::GetFieldsNames<T> {} ();
			const auto& boundFields = Util::Map (fields, [] (const QString& str) { return ':' + str; });

			return { table, db, fields, boundFields };
		}
	}

	template<typename T>
	struct ObjectInfo : detail::ObjectInfoFKeysHelper<T>
	{
		std::function<QList<T> ()> DoSelectAll_;
		detail::AdaptInsert<T> DoInsert_;
		detail::AdaptUpdate<T> DoUpdate_;
		detail::AdaptDelete<T> DoDelete_;

		detail::SelectByFieldsWrapper<T> DoSelectByFields_;
		detail::SelectOneByFieldsWrapper<T> DoSelectOneByFields_;
		detail::DeleteByFieldsWrapper<T> DoDeleteByFields_;

		ObjectInfo (decltype (DoSelectAll_) doSel,
				decltype (DoInsert_) doIns,
				decltype (DoUpdate_) doUpdate,
				decltype (DoDelete_) doDelete,
				decltype (DoSelectByFields_) selectByFields,
				decltype (DoSelectOneByFields_) selectOneByFields,
				decltype (DoDeleteByFields_) deleteByFields)
		: DoSelectAll_ (doSel)
		, DoInsert_ (doIns)
		, DoUpdate_ (doUpdate)
		, DoDelete_ (doDelete)
		, DoSelectByFields_ (selectByFields)
		, DoSelectOneByFields_ (selectOneByFields)
		, DoDeleteByFields_ (deleteByFields)
		{
		}
	};

	template<typename T>
	ObjectInfo<T> Adapt (const QSqlDatabase& db)
	{
		const auto& cachedData = detail::BuildCachedFieldsData<T> (db);

		if (db.record (cachedData.Table_).isEmpty ())
			RunTextQuery (db, detail::AdaptCreateTable<T> (cachedData));

		const auto& selectr = detail::AdaptSelectAll<T> (cachedData);
		const auto& insertr = detail::AdaptInsert<T> (cachedData);
		const auto& updater = detail::AdaptUpdate<T> (cachedData);
		const auto& deleter = detail::AdaptDelete<T> (cachedData);

		const auto& selectByVal = detail::AdaptSelectFields<T> (cachedData);
		const auto& selectOneByVal = detail::AdaptSelectOneFields<T> (cachedData);
		const auto& deleteByVal = detail::AdaptDeleteFields<T> (cachedData);

		ObjectInfo<T> info
		{
			selectr,
			insertr,
			updater,
			deleter,
			selectByVal,
			selectOneByVal,
			deleteByVal
		};

		detail::AdaptSelectRef<T> (cachedData, info);

		return info;
	}

	template<typename T>
	using ObjectInfo_ptr = std::shared_ptr<ObjectInfo<T>>;

	template<typename T>
	ObjectInfo_ptr<T> AdaptPtr (const QSqlDatabase& db)
	{
		return std::make_shared<ObjectInfo<T>> (Adapt<T> (db));
	}
}
}
}
