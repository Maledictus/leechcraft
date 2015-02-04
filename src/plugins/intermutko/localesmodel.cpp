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
#include "util.h"

namespace LeechCraft
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
		if (role != Qt::DisplayRole)
			return {};

		const auto& entry = Locales_.value (index.row ());
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
}
}
