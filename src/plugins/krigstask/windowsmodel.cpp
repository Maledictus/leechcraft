/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "windowsmodel.h"
#include <QtDebug>
#include <QIcon>
#include <QFrame>
#include <util/qml/widthiconprovider.h>
#include <util/x11/xwrapper.h>

#include <X11/extensions/Xcomposite.h>

namespace LC
{
namespace Krigstask
{
	class TaskbarImageProvider : public Util::WidthIconProvider
	{
		const QIcon DefaultIcon_;
		QHash<QString, QIcon> Icons_;
	public:
		TaskbarImageProvider (const QIcon& def)
		: DefaultIcon_ (def)
		{
		}

		void SetIcon (const QString& wid, const QIcon& icon)
		{
			Icons_ [wid] = icon;
		}

		void RemoveIcon (const QString& wid)
		{
			Icons_.remove (wid);
		}

		QIcon GetIcon (const QStringList& path)
		{
			const auto& wid = path.value (0);
			auto res = Icons_.value (wid);
			if (res.isNull ())
				res = DefaultIcon_;
			return res;
		}
	};

	WindowsModel::WindowsModel (QObject *parent)
	: RoleNamesMixin<QAbstractItemModel> (parent)
	, CurrentDesktop_ (Util::XWrapper::Instance ().GetCurrentDesktop ())
	, ImageProvider_ (new TaskbarImageProvider (QIcon::fromTheme ("xorg")))
	{
		auto& w = Util::XWrapper::Instance ();
		auto windows = w.GetWindows ();
		for (auto wid : windows)
			AddWindow (wid, w);

		connect (&w,
				SIGNAL (windowListChanged ()),
				this,
				SLOT (updateWinList ()));
		connect (&w,
				SIGNAL (activeWindowChanged ()),
				this,
				SLOT (updateActiveWindow ()));

		connect (&w,
				SIGNAL (windowNameChanged (ulong)),
				this,
				SLOT (updateWindowName (ulong)));
		connect (&w,
				SIGNAL (windowIconChanged (ulong)),
				this,
				SLOT (updateWindowIcon (ulong)));
		connect (&w,
				SIGNAL (windowStateChanged (ulong)),
				this,
				SLOT (updateWindowState (ulong)));
		connect (&w,
				SIGNAL (windowActionsChanged (ulong)),
				this,
				SLOT (updateWindowActions (ulong)));
		connect (&w,
				SIGNAL (windowDesktopChanged (ulong)),
				this,
				SLOT (updateWindowDesktop (ulong)));
		connect (&w,
				SIGNAL (desktopChanged ()),
				this,
				SLOT (updateCurrentDesktop ()));

		QHash<int, QByteArray> roleNames;
		roleNames [Role::WindowName] = "windowName";
		roleNames [Role::WindowID] = "windowID";
		roleNames [Role::IconGenID] = "iconGenID";
		roleNames [Role::IsCurrentDesktop] = "isCurrentDesktop";
		roleNames [Role::IsActiveWindow] = "isActiveWindow";
		roleNames [Role::IsMinimizedWindow] = "isMinimizedWindow";
		setRoleNames (roleNames);

		int eventBase, errorBase;
		if (XCompositeQueryExtension (w.GetDisplay (), &eventBase, &errorBase))
		{
			int major = 0, minor = 2;
			XCompositeQueryVersion (w.GetDisplay (), &major, &minor);

			if (major > 0 || minor >= 2)
				qDebug () << "all good, has NamePixmap";
		}
	}

	QQuickImageProvider* WindowsModel::GetImageProvider () const
	{
		return ImageProvider_;
	}

	int WindowsModel::columnCount (const QModelIndex&) const
	{
		return 1;
	}

	int WindowsModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : Windows_.size ();
	}

	QModelIndex WindowsModel::index (int row, int column, const QModelIndex& parent) const
	{
		return hasIndex (row, column, parent) ?
				createIndex (row, column) :
				QModelIndex {};
	}

	QModelIndex WindowsModel::parent (const QModelIndex&) const
	{
		return {};
	}

	QVariant WindowsModel::data (const QModelIndex& index, int role) const
	{
		const auto row = index.row ();
		const auto& item = Windows_.at (row);

		switch (role)
		{
		case Qt::DisplayRole:
		case Role::WindowName:
			return item.Title_;
		case Qt::DecorationRole:
			return item.Icon_;
		case Role::WindowID:
			return QString::number (item.WID_);
		case Role::IsCurrentDesktop:
			return item.DesktopNum_ == CurrentDesktop_ ||
					item.DesktopNum_ == static_cast<int> (0xFFFFFFFF);
		case Role::IconGenID:
			return QString::number (item.IconGenID_);
		case Role::IsActiveWindow:
			return item.IsActive_;
		case Role::IsMinimizedWindow:
			return item.State_.testFlag (Util::WinStateFlag::Hidden);
		}

		return {};
	}

	void WindowsModel::AddWindow (ulong wid, Util::XWrapper& w)
	{
		if (!w.ShouldShow (wid))
			return;

		const auto& icon = w.GetWindowIcon (wid);
		Windows_.append ({
					wid,
					w.GetWindowTitle (wid),
					icon,
					0,
					w.GetActiveApp () == wid,
					w.GetWindowDesktop (wid),
					w.GetWindowState (wid),
					w.GetWindowActions (wid)
				});
		ImageProvider_->SetIcon (QString::number (wid), icon);

		w.Subscribe (wid);
	}

	QList<WindowsModel::WinInfo>::iterator WindowsModel::FindWinInfo (Window wid)
	{
		return std::find_if (Windows_.begin (), Windows_.end (),
				[&wid] (const WinInfo& info) { return info.WID_ == wid; });
	}

	void WindowsModel::UpdateWinInfo (Window w, std::function<void (WinInfo&)> updater)
	{
		auto pos = FindWinInfo (w);
		if (pos == Windows_.end ())
			return;

		updater (*pos);

		const auto& idx = createIndex (std::distance (Windows_.begin (), pos), 0);
		emit dataChanged (idx, idx);
	}

	void WindowsModel::updateWinList ()
	{
		auto& w = Util::XWrapper::Instance ();

		QSet<Window> known;
		for (const auto& info : Windows_)
			known << info.WID_;

		auto current = w.GetWindows ();
		current.erase (std::remove_if (current.begin (), current.end (),
					[&w] (Window wid) { return !w.ShouldShow (wid); }), current.end ());

		for (auto i = current.begin (); i != current.end (); )
		{
			if (known.remove (*i))
				i = current.erase (i);
			else
				++i;
		}

		for (auto wid : known)
		{
			const auto pos = std::find_if (Windows_.begin (), Windows_.end (),
					[&wid] (const WinInfo& info) { return info.WID_ == wid; });
			const auto dist = std::distance (Windows_.begin (), pos);
			beginRemoveRows ({}, dist, dist);
			Windows_.erase (pos);
			endRemoveRows ();

			ImageProvider_->RemoveIcon (QString::number (wid));
		}

		if (!current.isEmpty ())
		{
			beginInsertRows ({}, Windows_.size (), Windows_.size () + current.size () - 1);
			for (auto wid : current)
				AddWindow (wid, w);
			endInsertRows ();
		}
	}

	void WindowsModel::updateActiveWindow ()
	{
		auto& w = Util::XWrapper::Instance ();
		const auto active = w.GetActiveApp ();
		if (!active || !w.ShouldShow (active))
			return;

		for (auto i = 0; i < Windows_.size (); ++i)
		{
			auto& info = Windows_ [i];
			if ((info.WID_ == active) == info.IsActive_)
				continue;

			info.IsActive_ = info.WID_ == active;
			emit dataChanged (createIndex (i, 0), createIndex (i, 0));
		}
	}

	void WindowsModel::updateWindowName (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info)
					{ info.Title_ = Util::XWrapper::Instance ().GetWindowTitle (w); });
	}

	void WindowsModel::updateWindowIcon (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) -> void
				{
					info.Icon_ = Util::XWrapper::Instance ().GetWindowIcon (w);
					++info.IconGenID_;
				});
	}

	void WindowsModel::updateWindowState (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info) -> void
				{
					info.State_ = Util::XWrapper::Instance ().GetWindowState (w);
					if (info.State_.testFlag (Util::WinStateFlag::Hidden))
						info.IsActive_ = false;
				});
	}

	void WindowsModel::updateWindowActions (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info)
					{ info.Actions_ = Util::XWrapper::Instance ().GetWindowActions (w); });
	}

	void WindowsModel::updateWindowDesktop (ulong w)
	{
		UpdateWinInfo (w,
				[&w] (WinInfo& info)
					{ info.DesktopNum_ = Util::XWrapper::Instance ().GetWindowDesktop (w); });
	}

	void WindowsModel::updateCurrentDesktop ()
	{
		CurrentDesktop_ = Util::XWrapper::Instance ().GetCurrentDesktop ();
		if (!Windows_.isEmpty ())
			emit dataChanged (createIndex (0, 0), createIndex (Windows_.size () - 1, 0));
	}
}
}
