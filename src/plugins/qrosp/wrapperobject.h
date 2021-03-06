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

#ifndef PLUGINS_QROSP_WRAPPERS_WRAPPEROBJECT_H
#define PLUGINS_QROSP_WRAPPERS_WRAPPEROBJECT_H
#include <memory>
#include <QObject>
#include <qross/core/action.h>
#include <interfaces/iinfo.h>
#include <interfaces/ijobholder.h>
#include <interfaces/iplugin2.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ihavetabs.h>

class QTranslator;

namespace LC
{
namespace Qrosp
{
	class WrapperObject : public QObject
						, public IInfo
						, public IEntityHandler
						, public IJobHolder
						, public IPlugin2
						, public IActionsExporter
						, public IHaveTabs
	{
		QString Type_;
		QString Path_;
		mutable Qross::Action *ScriptAction_;

		QList<QString> Interfaces_;

		QMetaObject *ThisMetaObject_;
		QMap<int, QMetaMethod> Index2MetaMethod_;
		QMap<int, QByteArray> Index2ExportedSignatures_;

		std::shared_ptr<QTranslator> Translator_;
	public:
		WrapperObject (const QString&, const QString&);
		virtual ~WrapperObject ();

		const QString& GetType () const;
		const QString& GetPath () const;

		// MOC hacks
		virtual void* qt_metacast (const char*);
		virtual const QMetaObject* metaObject () const;
		virtual int qt_metacall (QMetaObject::Call, int, void**);

		// IInfo
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;
		QStringList Provides () const;
		QStringList Needs () const;
		QStringList Uses () const;
		void SetProvider (QObject*, const QString&);

		// IEntityHandler
		EntityTestHandleResult CouldHandle (const LC::Entity&) const;
		void Handle (LC::Entity);

		// IJobHolder
		QAbstractItemModel* GetRepresentation () const;

		// IToolBarEmbedder
		QList<QAction*> GetActions (ActionsEmbedPlace) const;

		// IPlugin2
		QSet<QByteArray> GetPluginClasses () const;

		// IHaveTabs
		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		// Signals hacks
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);
		void gotActions (QList<QAction*>, LC::ActionsEmbedPlace);
	private:
		template<typename T>
		struct Call
		{
			Qross::Action *ScriptAction_;

			Call (Qross::Action *sa) : ScriptAction_ (sa) {}

			T operator() (const QString& name,
					const QVariantList& args = QVariantList ()) const
			{
				if (!ScriptAction_->functionNames ().contains (name))
					return T ();

				QVariant result = ScriptAction_->callFunction (name, args);
				if (result.canConvert<T> ())
					return result.value<T> ();

				qWarning () << Q_FUNC_INFO
						<< "unable to unwrap result"
						<< result;
				return T ();
			}
		};

		void LoadScriptTranslations ();
		void InitScript ();
		void InitQTS ();
		void BuildMetaObject ();
	};

	template<typename T>
	struct WrapperObject::Call<T*>
	{
		Qross::Action *ScriptAction_;

		Call (Qross::Action *sa) : ScriptAction_ (sa) {}

		T* operator() (const QString& name,
				const QVariantList& args = QVariantList ()) const
		{
			if (!ScriptAction_->functionNames ().contains (name))
				return 0;

			QVariant result = ScriptAction_->callFunction (name, args);
			if (result.canConvert<T*> ())
				return result.value<T*> ();
			else if (result.canConvert<QObject*> ())
			{
				QObject *object = result.value<QObject*> ();
				return qobject_cast<T*> (object);
			}

			qDebug () << Q_FUNC_INFO
					<< "unable to unwrap result"
					<< result;
			return 0;
		}
	};

	template<>
	struct WrapperObject::Call<void>
	{
		Qross::Action *ScriptAction_;

		Call (Qross::Action *sa) : ScriptAction_ (sa) {}

		void operator() (const QString&,
				const QVariantList& = QVariantList ()) const;
	};

	template<>
	struct WrapperObject::Call<QVariant>
	{
		Qross::Action *ScriptAction_;

		Call (Qross::Action *sa) : ScriptAction_ (sa) {}

		QVariant operator() (const QString& name,
				const QVariantList& args) const;
	};
}
}

#endif
