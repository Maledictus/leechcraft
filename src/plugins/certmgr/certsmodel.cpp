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

#include "certsmodel.h"
#include <QtDebug>

namespace LeechCraft
{
namespace CertMgr
{
	CertsModel::CertsModel (QObject *parent)
	: QAbstractItemModel { parent }
	{
	}

	QModelIndex CertsModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		if (!parent.isValid ())
			return createIndex (row, column, -1);

		if (static_cast<int> (parent.internalId ()) != -1)
			return {};

		auto id = static_cast<quint32> (parent.row ());
		return createIndex (row, column, id);
	}

	QModelIndex CertsModel::parent (const QModelIndex& child) const
	{
		if (!child.isValid ())
			return {};

		auto id = static_cast<int> (child.internalId ());
		if (id == -1)
			return {};

		return index (id, 0, {});
	}

	int CertsModel::columnCount (const QModelIndex&) const
	{
		return 1;
	}

	int CertsModel::rowCount (const QModelIndex& parent) const
	{
		if (!parent.isValid ())
			return Issuer2Certs_.size ();

		const auto id = static_cast<int> (parent.internalId ());
		return id == -1 ?
				Issuer2Certs_.value (parent.row ()).second.size () :
				0;
	}

	QVariant CertsModel::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid ())
			return {};

		const auto id = static_cast<int> (index.internalId ());

		if (id == -1)
			switch (role)
			{
			case Qt::DisplayRole:
				return { Issuer2Certs_.value (index.row ()).first };
			default:
				return {};
			}

		const auto& cert = Issuer2Certs_.value (id).second.value (index.row ());

		switch (role)
		{
		case Qt::DisplayRole:
			return cert.subjectInfo (QSslCertificate::CommonName);
		default:
			return {};
		}
	}

	void CertsModel::AddCert (const QSslCertificate& cert)
	{
		const auto pos = CreateListPosForCert (cert);

		const auto& parentIndex = index (std::distance (Issuer2Certs_.begin (), pos), 0, {});

		beginInsertRows (parentIndex, pos->second.size (), pos->second.size ());
		pos->second << cert;
		endInsertRows ();
	}

	void CertsModel::RemoveCert (const QSslCertificate& cert)
	{
	}

	void CertsModel::ResetCerts (const QList<QSslCertificate>& certs)
	{
		beginResetModel ();

		Issuer2Certs_.clear ();

		for (const auto& cert : certs)
			CreateListForCert (cert) << cert;

		endResetModel ();
	}

	auto CertsModel::CreateListPosForCert (const QSslCertificate& cert) -> CertsDict_t::iterator
	{
		auto issuer = cert.issuerInfo (QSslCertificate::Organization);
		if (issuer.isEmpty ())
			issuer = cert.issuerInfo (QSslCertificate::CommonName);

		auto pos = std::lower_bound (Issuer2Certs_.begin (), Issuer2Certs_.end (), issuer,
				[] (decltype (Issuer2Certs_.at (0)) item, const QString& issuer)
					{ return QString::localeAwareCompare (item.first.toLower (), issuer.toLower ()) < 0; });

		if (pos != Issuer2Certs_.end () && pos->first.toLower () == issuer.toLower ())
			return pos;

		const auto row = std::distance (Issuer2Certs_.begin (), pos);
		beginInsertRows ({}, row, row);
		pos = Issuer2Certs_.insert (pos, { issuer, {} });
		endInsertRows ();

		return pos;
	}

	QList<QSslCertificate>& CertsModel::CreateListForCert (const QSslCertificate& cert)
	{
		return CreateListPosForCert (cert)->second;
	}
}
}
