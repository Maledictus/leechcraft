/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "polldialog.h"
#include <QMessageBox>
#include <QStandardItemModel>
#include <QtDebug>

namespace LC
{
namespace Blogique
{
namespace Metida
{
	static const int ScaleFromDefaultValue = 1;
	static const int ScaleToDefaultValue = 10;
	static const int ScaleByDefaultValue = 1;

	PollDialog::PollDialog (QWidget *parent)
	: QDialog (parent)
	, CheckModel_ (new QStandardItemModel (this))
	, RadioModel_ (new QStandardItemModel (this))
	, DropModel_ (new QStandardItemModel (this))
	, PollTypeModel_ (new QStandardItemModel (this))
	{
		Ui_.setupUi (this);

		Ui_.Fields_->setModel (CheckModel_);
		Ui_.PollType_->setModel (PollTypeModel_);

		QStandardItem *check = new QStandardItem (tr ("Check boxes"));
		check->setFlags (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		check->setData ("check");
		check->setData (Qt::Unchecked, Qt::CheckStateRole);
		PollTypeModel_->appendRow (check);
		QStandardItem *radio = new QStandardItem (tr ("Radio buttons"));
		radio->setFlags (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		radio->setData ("radio");
		radio->setData (Qt::Unchecked, Qt::CheckStateRole);
		PollTypeModel_->appendRow (radio);
		QStandardItem *drop = new QStandardItem (tr ("Dropdown"));
		drop->setFlags (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		drop->setData ("drop");
		drop->setData (Qt::Unchecked, Qt::CheckStateRole);
		PollTypeModel_->appendRow (drop);
		QStandardItem *text = new QStandardItem (tr ("Text entry"));
		text->setFlags (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		text->setData ("text");
		text->setData (Qt::Unchecked, Qt::CheckStateRole);
		PollTypeModel_->appendRow (text);
		QStandardItem *scale = new QStandardItem (tr ("Scale"));
		scale->setFlags (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		scale->setData ("scale");
		scale->setData (Qt::Unchecked, Qt::CheckStateRole);
		PollTypeModel_->appendRow (scale);

		Ui_.PollQuestion_->setToolTip (tr ("Allowed tags: %1.")
					.arg ("&lt;a>, &lt;b>, &lt;i>, &lt;em>, &lt;strong>, &lt;u>, &lt;img>, &lt;lj user>"));
		connect (Ui_.PollType_->view (),
				SIGNAL (clicked (QModelIndex)),
				this,
				SLOT (handleItemActivated (QModelIndex)));
		connect (PollTypeModel_,
				SIGNAL (itemChanged (QStandardItem*)),
				this,
				SLOT (handleItemChanged (QStandardItem*)));
	}

	QString PollDialog::GetPollName () const
	{
		return Ui_.PollName_->text ();
	}

	QString PollDialog::GetWhoCanView () const
	{
		switch (Ui_.PollView_->currentIndex ())
		{
		case ViewOnlyFriends:
			return "friends";
		case ViewOnlyMe:
			return "none";
		default:
			return "all";
		}
	}

	QString PollDialog::GetWhoCanVote () const
	{
		switch (Ui_.PollVote_->currentIndex ())
		{
			case VoteOnlyFriends:
				return "friends";
			default:
				return "all";
		}
	}

	QStringList PollDialog::GetPollTypes () const
	{
		QStringList result;
		for (int i = 0; i < PollTypeModel_->rowCount (); ++i)
		{
			auto item = PollTypeModel_->item (i);
			if (item->checkState () == Qt::Checked)
				result << item->data ().toString ();
		}

		return result;
	}

	QString PollDialog::GetPollQuestion (const QString& type) const
	{
		return Type2Question_.value (type);
	}

	QVariantMap PollDialog::GetPollFields (const QString& pollType) const
	{
		QVariantMap result;
		if (pollType == "check")
			result = GetFields (CheckModel_);
		else if (pollType == "radio")
			result =  GetFields (RadioModel_);
		else if (pollType == "drop")
			result =  GetFields (DropModel_);
		else if (pollType == "text")
		{
			result ["size"] = Ui_.TextSize_->value ();
			result ["maxLength"] = Ui_.TextMaxLength_->value ();
		}
		else if (pollType == "scale")
		{
			result ["from"] = Ui_.ScaleFrom_->value ();
			result ["to"] = Ui_.ScaleTo_->value ();
			result ["by"] = Ui_.ScaleBy_->value ();
		}

		return result;
	}


	int PollDialog::GetScaleFrom () const
	{
		return Ui_.ScaleFrom_->value ();
	}

	int PollDialog::GetScaleTo () const
	{
		return Ui_.ScaleTo_->value ();
	}

	int PollDialog::GetScaleBy () const
	{
		return Ui_.ScaleBy_->value ();
	}

	int PollDialog::GetTextSize () const
	{
		return Ui_.TextSize_->value ();
	}

	int PollDialog::GetTextMaxLength () const
	{
		return Ui_.TextMaxLength_->value ();
	}

	void PollDialog::accept ()
	{
		int count = 0;
		QHash<QString, int> type2FieldsCount;
		for (int i = 0; i < PollTypeModel_->rowCount (); ++i)
		{
			auto item = PollTypeModel_->item (i);
			if (item->checkState () == Qt::Checked)
			{
				++count;
				const QString& type = item->data ().toString ();
				if (type == "check")
					type2FieldsCount [item->data ().toString ()] = GetFields (CheckModel_).count ();
				else if (type == "radio")
					type2FieldsCount [item->data ().toString ()] = GetFields (RadioModel_).count ();
				else if (type == "drop")
					type2FieldsCount [item->data ().toString ()] = GetFields (DropModel_).count ();
			}
		}
		if (!count)
		{
			QMessageBox::warning (this,
					"LeechCraft",
					tr ("Poll should have at least one question."),
					QMessageBox::Ok);
			return;
		}
		else
		{
			if (GetPollTypes ().contains ("scale") &&
					!IsScaleValuesAreValid ())
			{
				const int res = QMessageBox::warning (this,
						"LeechCraft",
						tr ("Scaled values are invalid. Default values will be used."),
						QMessageBox::Ok | QMessageBox::Cancel);
				if (res == QMessageBox::Ok)
				{
					Ui_.ScaleFrom_->setValue (ScaleFromDefaultValue);
					Ui_.ScaleTo_->setValue (ScaleToDefaultValue);
					Ui_.ScaleBy_->setValue (ScaleByDefaultValue);
				}
			}
		}

		QDialog::accept ();
	}

	QVariantMap PollDialog::GetFields (QStandardItemModel *model) const
	{
		QVariantMap result;
		for (int i = 0; i < model->rowCount (); ++i)
		{
			const auto& text = model->item (i)->text ();
			if (!text.isEmpty ())
				result [QString ("field%1").arg (i)] = text;
		}
		return result;
	}

	bool PollDialog::IsScaleValuesAreValid () const
	{
		return (Ui_.ScaleTo_->value () - Ui_.ScaleFrom_->value ()) / Ui_.ScaleBy_->value () < 20;
	}

	void PollDialog::on_AddField__released ()
	{
		QStandardItemModel *model = 0;
		switch (Ui_.PollType_->currentIndex ())
		{
		case CheckBoxes:
			model = CheckModel_;
			break;
		case RadioButtons:
			model = RadioModel_;
			break;
		case DropdownBox:
			model = DropModel_;
			break;
		default:
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown poll type";
			return;
		}
		}
		model->appendRow (new QStandardItem (tr ("Field %1").arg (model->rowCount () + 1)));
	}

	void PollDialog::on_RemoveField__released ()
	{
		const auto& index = Ui_.Fields_->currentIndex ();
		if (!index.isValid ())
			return;
		switch (Ui_.PollType_->currentIndex ())
		{
			case CheckBoxes:
				CheckModel_->removeRow (index.row ());
				break;
			case RadioButtons:
				RadioModel_->removeRow (index.row ());;
				break;
			case DropdownBox:
				DropModel_->removeRow (index.row ());;
				break;
			default:
				break;
		}
	}

	void PollDialog::on_PollType__currentIndexChanged (int index)
	{
		switch (index)
		{
			case CheckBoxes:
				Ui_.OptionsStack_->setCurrentIndex (0);
				Ui_.Fields_->setModel (CheckModel_);
				Ui_.PollQuestion_->setText (Type2Question_.value ("check"));
				break;
			case RadioButtons:
				Ui_.OptionsStack_->setCurrentIndex (0);
				Ui_.Fields_->setModel (RadioModel_);
				Ui_.PollQuestion_->setText (Type2Question_.value ("radio"));
				break;
			case DropdownBox:
				Ui_.OptionsStack_->setCurrentIndex (0);
				Ui_.Fields_->setModel (DropModel_);
				Ui_.PollQuestion_->setText (Type2Question_.value ("drop"));
				break;
			case TextEntry:
				Ui_.OptionsStack_->setCurrentIndex (1);
				Ui_.PollQuestion_->setText (Type2Question_.value ("text"));
				break;
			case Scale:
				Ui_.OptionsStack_->setCurrentIndex (2);
				Ui_.PollQuestion_->setText (Type2Question_.value ("scale"));
				break;
			default:
				break;
		}
	}

	void PollDialog::handleItemActivated (const QModelIndex& index)
	{
		if (!ItemIsChanged_)
		{
			Ui_.PollType_->setCurrentIndex (index.row ());
			Ui_.PollType_->hidePopup ();
		}
		ItemIsChanged_ = false;
	}

	void PollDialog::handleItemChanged (QStandardItem*)
	{
		ItemIsChanged_ = true;
	}

	void PollDialog::on_PollQuestion__textChanged (const QString& text)
	{
		switch (Ui_.PollType_->currentIndex ())
		{
		case CheckBoxes:
			Type2Question_ ["check"] = text;
			break;
		case RadioButtons:
			Type2Question_ ["radio"] = text;
			break;
		case DropdownBox:
			Type2Question_ ["drop"] = text;
			break;
		case TextEntry:
			Type2Question_ ["text"] = text;
			break;
		case Scale:
			Type2Question_ ["scale"] = text;
			break;
		default:
			break;
		}
	}
}
}
}
