/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "menumodeladapter.h"
#include <QAbstractItemModel>
#include <QMenu>
#include <QtDebug>

namespace LC::Util
{
	namespace
	{
		class MenuModelManager : public QObject
		{
			QMenu& Menu_;
			QAbstractItemModel& Model_;
		public:
			explicit MenuModelManager (QMenu& menu, QAbstractItemModel& model)
			: QObject { &menu }
			, Menu_ { menu }
			, Model_ { model }
			{
				connect (&model,
						&QObject::destroyed,
						this,
						&QObject::deleteLater);

				RegenerateMenu ();

				connect (&model,
						&QAbstractItemModel::modelReset,
						&menu,
						&QMenu::clear);
				connect (&model,
						&QAbstractItemModel::rowsMoved,
						this,
						&MenuModelManager::MoveRows);
				connect (&model,
						&QAbstractItemModel::rowsRemoved,
						this,
						&MenuModelManager::RemoveRows);
				connect (&model,
						&QAbstractItemModel::rowsInserted,
						this,
						&MenuModelManager::InsertRows);
			}
		private:
			void RegenerateMenu ()
			{
				Menu_.clear ();

				if (const auto rc = Model_.rowCount ())
					Menu_.addActions (MakeActionsForRange (0, rc - 1));
			}

			void MoveRows (const QModelIndex& parent, int start, int end,
					const QModelIndex& destParent, int row)
			{
				if (!CheckRootParent (parent, destParent) || !CheckIndexes (start, end, row))
					return;

				const auto& actions = Menu_.actions ();
				const auto& moved = actions.mid (start, end - start + 1);
				const auto& target = actions [row];
				Menu_.insertActions (target, moved);
			}

			void RemoveRows (const QModelIndex& parent, int first, int last)
			{
				if (!CheckRootParent (parent) || !CheckIndexes (first, last))
					return;

				const auto& actions = Menu_.actions ();
				for (const auto action : actions.mid (first, last - first + 1))
					Menu_.removeAction (action);
			}

			QList<QAction*> MakeActionsForRange (int first, int last)
			{
				QList<QAction*> actions;
				actions.reserve (last - first + 1);
				for (int i = first; i <= last; ++i)
				{
					const auto& idx = Model_.index (i, 0);
					const auto& icon = idx.data (Qt::DecorationRole).value<QIcon> ();
					const auto& text = idx.data (Qt::DisplayRole).toString ();
					const auto& tooltip = idx.data (Qt::ToolTipRole).toString ();

					const auto act = new QAction { icon, text, &Menu_ };
					act->setToolTip (tooltip);
					actions << act;
				}
				return actions;
			}

			void InsertRows (const QModelIndex& parent, int first, int last)
			{
				if (!CheckRootParent (parent))
					return;

				const auto& actions = MakeActionsForRange (first, last);
				const auto curActionsCount = Menu_.actions ().size ();
				if (first == curActionsCount)
					Menu_.addActions (actions);
				else if (first < curActionsCount)
					Menu_.insertActions (Menu_.actions () [first], actions);
				else
				{
					qWarning () << "invalid actions position" << &Menu_ << &Model_ << curActionsCount << first << last;
					qDeleteAll (actions);
				}
			}

			template<typename... Idxs>
			bool CheckRootParent (const Idxs&... idxes)
			{
				if ((idxes.isValid () || ...))
				{
					((qWarning () << "ignoring row operations for non-root indexes" << &Menu_ << &Model_) << ... << idxes);
					return false;
				}

				return true;
			}

			template<typename... Idxs>
			bool CheckIndexes (Idxs... idxes)
			{
				if (((idxes >= Menu_.actions ().size ()) || ...))
				{
					((qWarning () << "out of bounds indexes" << &Menu_ << &Model_) << ... << idxes);
					RegenerateMenu ();
					return false;
				}

				return true;
			}
		};
	}

	void SetMenuModel (QMenu& menu, QAbstractItemModel& model)
	{
		new MenuModelManager { menu, model };
	}
}
