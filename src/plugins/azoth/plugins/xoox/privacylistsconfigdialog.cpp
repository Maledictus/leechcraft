/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "privacylistsconfigdialog.h"
#include <memory>
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardItemModel>
#include <QtDebug>
#include <util/sll/functional.h>
#include "privacylistsitemdialog.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	PrivacyListsConfigDialog::PrivacyListsConfigDialog (PrivacyListsManager *mgr, QWidget *parent)
	: QDialog (parent)
	, Manager_ (mgr)
	, Model_ (new QStandardItemModel (this))
	{
		Model_->setHorizontalHeaderLabels ({ tr ("Type"), tr ("Value"), tr ("Action"), tr ("Stanzas") });

		Ui_.setupUi (this);
		Ui_.RulesTree_->setModel (Model_);

		ReinitModel ();

		connect (Manager_,
				SIGNAL (listError (QString)),
				this,
				SLOT (handleError (QString)));

		QueryLists ();

		setAttribute (Qt::WA_DeleteOnClose);
	}

	void PrivacyListsConfigDialog::QueryLists ()
	{
		Ui_.StatusLabel_->setText (tr ("Fetching names of privacy lists..."));

		connect (Manager_,
				SIGNAL (gotLists (QStringList, QString, QString)),
				this,
				SLOT (handleGotLists (QStringList, QString, QString)));

		Manager_->QueryLists ();
	}

	void PrivacyListsConfigDialog::QueryList (const QString& list)
	{
		if (Lists_.contains (list))
		{
			HandleGotList (Lists_ [list]);
			return;
		}

		Ui_.StatusLabel_->setText (tr ("Fetching list %1...").arg (list));

		Manager_->QueryList (list,
				{
					[] (const QXmppIq&) { qWarning () << Q_FUNC_INFO << "unable to fetch list"; },
					Util::BindMemFn (&PrivacyListsConfigDialog::HandleGotList, this)
				});
	}

	void PrivacyListsConfigDialog::AddListToBoxes (const QString& listName)
	{
		Ui_.ActiveList_->addItem (listName);
		Ui_.DefaultList_->addItem (listName);
		Ui_.ConfigureList_->addItem (listName);
	}

	void PrivacyListsConfigDialog::ReinitModel ()
	{
		if (const auto rc = Model_->rowCount ())
			Model_->removeRows (0, rc);
	}

	QList<QStandardItem*> PrivacyListsConfigDialog::ToRow (const PrivacyListItem& item) const
	{
		QList<QStandardItem*> modelItems;

		switch (item.GetType ())
		{
		case PrivacyListItem::Type::None:
			modelItems << new QStandardItem (tr ("None"));
			break;
		case PrivacyListItem::Type::Jid:
			modelItems << new QStandardItem (tr ("JID"));
			break;
		case PrivacyListItem::Type::Subscription:
			modelItems << new QStandardItem (tr ("Subscription"));
			break;
		case PrivacyListItem::Type::Group:
			modelItems << new QStandardItem (tr ("Group"));
			break;
		}

		modelItems << new QStandardItem (item.GetValue ());
		modelItems << new QStandardItem (item.GetAction () == PrivacyListItem::Action::Allow ?
					tr ("Allow") :
					tr ("Deny"));

		QStringList stanzasList;
		const PrivacyListItem::StanzaTypes types = item.GetStanzaTypes ();
		if (types == PrivacyListItem::STAll ||
				types == PrivacyListItem::STNone)
			stanzasList << tr ("All");
		else
		{
			if (types & PrivacyListItem::STMessage)
				stanzasList << tr ("Messages");
			if (types & PrivacyListItem::STPresenceIn)
				stanzasList << tr ("Incoming presences");
			if (types & PrivacyListItem::STPresenceOut)
				stanzasList << tr ("Outgoing presences");
			if (types & PrivacyListItem::STIq)
				stanzasList << tr ("IQ");
		}

		modelItems << new QStandardItem (stanzasList.join ("; "));

		return modelItems;
	}

	void PrivacyListsConfigDialog::accept ()
	{
		QDialog::accept ();

		for (const auto& pl : Lists_)
			Manager_->SetList (pl);

		Manager_->ActivateList (Ui_.ActiveList_->currentText (),
				PrivacyListsManager::ListType::Active);
		Manager_->ActivateList (Ui_.DefaultList_->currentText (),
				PrivacyListsManager::ListType::Default);
	}

	void PrivacyListsConfigDialog::reject ()
	{
		QDialog::reject ();
	}

	void PrivacyListsConfigDialog::on_ConfigureList__activated (int idx)
	{
		QueryList (Ui_.ConfigureList_->itemText (idx));
	}

	void PrivacyListsConfigDialog::on_AddButton__released ()
	{
		const QString& listName = QInputDialog::getText (this,
				"LeechCraft",
				tr ("Please enter the name of the new list"));
		if (listName.isEmpty ())
			return;

		Lists_ [listName] = PrivacyList (listName);
		AddListToBoxes (listName);

		ReinitModel ();

		Ui_.ConfigureList_->blockSignals (true);
		Ui_.ConfigureList_->setCurrentIndex (Ui_.ConfigureList_->findText (listName));
		Ui_.ConfigureList_->blockSignals (false);

		on_DefaultPolicy__currentIndexChanged (Ui_.DefaultList_->currentIndex ());
	}

	void PrivacyListsConfigDialog::on_RemoveButton__released ()
	{
		const QString& listName = Ui_.ConfigureList_->currentText ();
		if (listName.isEmpty ())
			return;

		if (Ui_.DefaultList_->currentText () == listName ||
				Ui_.ActiveList_->currentText () == listName)
		{
			QMessageBox::critical (this,
					"LeechCraft",
					tr ("Unable to delete a list that is currently "
						"active or selected as default one."));
			return;
		}

		if (QMessageBox::question (this,
					"LeechCraft",
					tr ("This list would be immediately and "
						"permanently deleted. Are you sure?"),
					QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		ReinitModel ();

		PrivacyList list (listName);
		Manager_->SetList (list);
		Lists_.remove (listName);
		Ui_.ConfigureList_->removeItem (Ui_.ConfigureList_->currentIndex ());
	}

	void PrivacyListsConfigDialog::on_DefaultPolicy__currentIndexChanged (int idx)
	{
		const QString& listName = Ui_.ConfigureList_->currentText ();
		if (listName.isEmpty ())
			return;

		const auto action = idx == 0 ?
				PrivacyListItem::Action::Allow :
				PrivacyListItem::Action::Deny;

		auto items = Lists_ [listName].GetItems ();
		if (!items.isEmpty () &&
				items.last ().GetType () == PrivacyListItem::Type::None)
			items.removeLast ();

		items.append ({ {}, PrivacyListItem::Type::None, action });

		Lists_ [listName].SetItems (items);
	}

	void PrivacyListsConfigDialog::on_AddRule__released ()
	{
		PrivacyListsItemDialog dia;
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& item = dia.GetItem ();
		Model_->appendRow (ToRow (item));

		auto& list = Lists_ [Ui_.ConfigureList_->currentText ()];
		auto items = list.GetItems ();
		items << item;
		list.SetItems (items);
	}

	void PrivacyListsConfigDialog::on_ModifyRule__released ()
	{
		const auto& index = Ui_.RulesTree_->currentIndex ();
		if (!index.isValid ())
			return;

		const int row = index.row ();

		auto& list = Lists_ [Ui_.ConfigureList_->currentText ()];
		auto items = list.GetItems ();

		PrivacyListsItemDialog dia;
		dia.SetItem (items.at (row));
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& item = dia.GetItem ();
		items [row] = item;
		list.SetItems (items);

		int column = 0;
		for (auto modelItem : ToRow (item))
			Model_->setItem (row, column++, modelItem);
	}

	void PrivacyListsConfigDialog::on_RemoveRule__released ()
	{
		const QModelIndex& index = Ui_.RulesTree_->currentIndex ();
		if (!index.isValid ())
			return;

		const int row = index.row ();
		Model_->removeRow (row);

		PrivacyList& list = Lists_ [Ui_.ConfigureList_->currentText ()];
		QList<PrivacyListItem> items = list.GetItems ();
		items.removeAt (row);
		list.SetItems (items);
	}

	void PrivacyListsConfigDialog::on_MoveUp__released ()
	{
		const QModelIndex& index = Ui_.RulesTree_->currentIndex ();
		if (!index.isValid () || index.row () < 1)
			return;

		Model_->insertRow (index.row () - 1, Model_->takeRow (index.row ()));
	}

	void PrivacyListsConfigDialog::on_MoveDown__released ()
	{
		const QModelIndex& index = Ui_.RulesTree_->currentIndex ();
		if (!index.isValid () || index.row () >= Model_->rowCount () - 1)
			return;

		Model_->insertRow (index.row (), Model_->takeRow (index.row () + 1));
	}

	void PrivacyListsConfigDialog::handleGotLists (const QStringList& lists,
			const QString& active, const QString& def)
	{
		disconnect (Manager_,
				SIGNAL (gotLists (QStringList, QString, QString)),
				this,
				SLOT (handleGotLists (QStringList, QString, QString)));

		Ui_.ConfigureList_->clear ();
		Ui_.ConfigureList_->addItems (lists);
		Ui_.ActiveList_->clear ();
		Ui_.ActiveList_->addItems (QStringList { {} } + lists);
		Ui_.DefaultList_->clear ();
		Ui_.DefaultList_->addItems (QStringList { {} } + lists);

		int idx = Ui_.ActiveList_->findText (active);
		if (idx >= 0)
			Ui_.ActiveList_->setCurrentIndex (idx);
		idx = Ui_.DefaultList_->findText (def);
		if (idx >= 0)
			Ui_.DefaultList_->setCurrentIndex (idx);

		Ui_.StatusLabel_->setText ({});

		if (!lists.isEmpty ())
			QueryList (lists.at (0));
	}

	void PrivacyListsConfigDialog::HandleGotList (const PrivacyList& list)
	{
		Ui_.StatusLabel_->setText ({});

		ReinitModel ();

		Lists_ [list.GetName ()] = list;

		auto items = list.GetItems ();
		if (!items.isEmpty () && items.last ().GetType () == PrivacyListItem::Type::None)
		{
			const auto& item = items.takeLast ();
			const auto idx = item.GetAction () == PrivacyListItem::Action::Allow ? 0 : 1;
			Ui_.DefaultPolicy_->setCurrentIndex (idx);
		}

		for (const auto& item : items)
			Model_->appendRow (ToRow (item));
	}

	void PrivacyListsConfigDialog::handleError (const QString& text)
	{
		QMessageBox::critical (this,
				"LeechCraft",
				tr ("Error fetching lists.") + " " + text);
	}
}
}
}
