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

#include "localesmodel.h"
#include <QtDebug>
#include <util/sll/util.h>
#include "util.h"

namespace LC
{
namespace Intermutko
{
	int LocalesModel::columnCount (const QModelIndex& parent) const
	{
		return parent.isValid () ?
				0 :
				Headers_.size ();
	}

	int LocalesModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ?
				0 :
				Locales_.size ();
	}

	QModelIndex LocalesModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (parent.isValid ())
			return {};

		return createIndex (row, column);
	}

	QModelIndex LocalesModel::parent (const QModelIndex&) const
	{
		return {};
	}

	QVariant LocalesModel::headerData (int section, Qt::Orientation orientation, int role) const
	{
		if (orientation == Qt::Vertical)
			return {};

		if (role != Qt::DisplayRole)
			return {};

		return Headers_.value (section);
	}

	QVariant LocalesModel::data (const QModelIndex& index, int role) const
	{
		const auto& entry = Locales_.value (index.row ());
		if (role == static_cast<int> (Role::LocaleEntry))
			return QVariant::fromValue (entry);

		if (role != Qt::DisplayRole && role != Qt::EditRole)
			return {};

		switch (static_cast<Column> (index.column ()))
		{
		case Column::Language:
			return QLocale::languageToString (entry.Language_);
		case Column::Country:
			return GetCountryName (entry.Language_, entry.Country_);
		case Column::Quality:
			return entry.Q_;
		case Column::Code:
			return GetDisplayCode (entry);
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown column"
				<< index;
		return {};
	}

	Qt::ItemFlags LocalesModel::flags (const QModelIndex& index) const
	{
		Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (index.column () != static_cast<int> (Column::Code))
			result |= Qt::ItemIsEditable;
		return result;
	}

	bool LocalesModel::setData (const QModelIndex& idx, const QVariant& value, int)
	{
		if (!idx.isValid ())
			return false;

		const auto dataChangedGuard = Util::MakeScopeGuard ([this, &idx]
				{
					emit dataChanged (index (idx.row (), 0), index (idx.row (), columnCount () - 1));
				});

		auto& entry = Locales_ [idx.row ()];

		switch (static_cast<Column> (idx.column ()))
		{
		case Column::Language:
			entry.Language_ = static_cast<QLocale::Language> (value.toInt ());
			return true;
		case Column::Country:
			entry.Country_ = static_cast<QLocale::Country> (value.toInt ());
			return true;
		case Column::Quality:
			entry.Q_ = value.toDouble ();
			return true;
		case Column::Code:
			return true;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown column"
				<< idx;
		return false;
	}

	const QList<LocaleEntry>& LocalesModel::GetEntries () const
	{
		return Locales_;
	}

	void LocalesModel::AddLocaleEntry (const LocaleEntry& entry)
	{
		beginInsertRows ({}, Locales_.size (), Locales_.size ());
		Locales_ << entry;
		endInsertRows ();
	}

	void LocalesModel::SetLocales (const QList<LocaleEntry>& entries)
	{
		if (const auto rc = Locales_.size ())
		{
			beginRemoveRows ({}, 0, rc - 1);
			Locales_.clear ();
			endRemoveRows ();
		}

		if (const auto rc = entries.size ())
		{
			beginInsertRows ({}, 0, rc - 1);
			Locales_ = entries;
			endInsertRows ();
		}
	}

	void LocalesModel::Remove (const QModelIndex& index)
	{
		if (!index.isValid ())
			return;

		const auto r = index.row ();
		beginRemoveRows ({}, r, r);
		Locales_.removeAt (r);
		endRemoveRows ();
	}

	void LocalesModel::MoveUp (const QModelIndex& index)
	{
		if (!index.isValid () || !index.row ())
			return;

		const auto r = index.row ();

		beginRemoveRows ({}, r - 1, r - 1);
		const auto& preRow = Locales_.takeAt (r - 1);
		endRemoveRows ();

		beginInsertRows ({}, r, r);
		Locales_.insert (r, preRow);
		endInsertRows ();
	}

	void LocalesModel::MoveDown (const QModelIndex& index)
	{
		if (!index.isValid () || index.row () == Locales_.size () - 1)
			return;

		const auto r = index.row ();
		beginRemoveRows ({}, r + 1, r + 1);
		const auto& postRow = Locales_.takeAt (r + 1);
		endRemoveRows ();

		beginInsertRows ({}, r, r);
		Locales_.insert (r, postRow);
		endInsertRows ();
	}
}
}
