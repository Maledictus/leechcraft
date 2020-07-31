/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "legacyformbuilder.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QVariant>
#include <QLineEdit>
#include <QtDebug>

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	namespace
	{
		void LineEditActorImpl (QWidget *form, const QXmppElement& elem,
				const QString& fieldLabel)
		{
			QLabel *label = new QLabel (fieldLabel);
			QLineEdit *edit = new QLineEdit (elem.value ());
			edit->setObjectName ("field");
			edit->setProperty ("FieldName", elem.tagName ());

			QHBoxLayout *lay = new QHBoxLayout (form);
			lay->addWidget (label);
			lay->addWidget (edit);
			qobject_cast<QVBoxLayout*> (form->layout ())->addLayout (lay);
		}

		void InstructionsActor (QWidget *form, const QXmppElement& elem)
		{
			QLabel *label = new QLabel (elem.value ());
			form->layout ()->addWidget (label);
		}
	}

	LegacyFormBuilder::LegacyFormBuilder ()
	: Widget_ (0)
	{
		Tag2Actor_ ["username"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Username:")); };
		Tag2Actor_ ["password"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Password:")); };
		Tag2Actor_ ["registered"] = [] (QWidget*, const QXmppElement&) {};
		Tag2Actor_ ["first"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("First name:")); };
		Tag2Actor_ ["last"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Last name:")); };
		Tag2Actor_ ["nick"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Nick:")); };
		Tag2Actor_ ["email"] = [] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("E-Mail:")); };
		Tag2Actor_ ["instructions"] = [] (QWidget *f, const QXmppElement& e)
			{ InstructionsActor (f, e); };
	}

	void LegacyFormBuilder::Clear ()
	{
		delete Widget_;
		Widget_ = nullptr;
	}

	QWidget* LegacyFormBuilder::CreateForm (const QXmppElement& containing,
			QWidget *parent)
	{
		Widget_ = new QWidget (parent);
		Widget_->setLayout (new QVBoxLayout ());

		QXmppElement element = containing.firstChildElement ();
		while (!element.isNull ())
		{
			const QString& tag = element.tagName ();

			if (!Tag2Actor_.contains (tag))
				qWarning () << Q_FUNC_INFO
						<< "unknown tag";
			else
				Tag2Actor_ [tag] (Widget_, element);

			element = element.nextSiblingElement ();
		}

		return Widget_;
	}

	QList<QXmppElement> LegacyFormBuilder::GetFilledChildren () const
	{
		QList<QXmppElement> result;
		if (!Widget_)
			return result;

		for (auto edit : Widget_->findChildren<QLineEdit*> ("field"))
		{
			QXmppElement elem;
			elem.setTagName (edit->property ("FieldName").toString ());
			elem.setValue (edit->text ());
			result << elem;
		}

		return result;
	}

	QString LegacyFormBuilder::GetUsername () const
	{
		if (!Widget_)
			return {};

		for (auto edit : Widget_->findChildren<QLineEdit*> ("field"))
			if (edit->property ("FieldName").toString () == "username")
				return edit->text ();

		return {};
	}

	QString LegacyFormBuilder::GetPassword () const
	{
		for (auto edit : Widget_->findChildren<QLineEdit*> ("field"))
			if (edit->property ("FieldName").toString () == "password")
				return edit->text ();

		return {};
	}
}
}
}
