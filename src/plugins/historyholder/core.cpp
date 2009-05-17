#include "core.h"
#include <typeinfo>
#include <QSettings>
#include <QTextCodec>
#include <QToolBar>
#include <QAction>
#include <QTreeView>
#include <plugininterface/structuresops.h>
#include <plugininterface/proxy.h>
#include "findproxy.h"

using LeechCraft::Util::Proxy;
using namespace LeechCraft::Plugins::HistoryHolder;

QDataStream& operator<< (QDataStream& out, const Core::HistoryEntry& e)
{
	quint16 version = 1;
	out << version;

	out << e.Entity_
		<< e.DateTime_;

	return out;
}

QDataStream& operator>> (QDataStream& in, Core::HistoryEntry& e)
{
	quint16 version;
	in >> version;
	if (version == 1)
	{
		in >> e.Entity_
			>> e.DateTime_;
	}
	else
	{
		qWarning () << Q_FUNC_INFO
			<< "unknown version"
			<< version;
	}
	return in;
}

LeechCraft::Plugins::HistoryHolder::Core::Core ()
: ToolBar_ (new QToolBar)
{
	Headers_ << tr ("Entity/location")
			<< tr ("Date")
			<< tr ("Tags");

	qRegisterMetaType<HistoryEntry> ("LeechCraft::Plugins::HistoryHolder::Core::HistoryEntry");
	qRegisterMetaTypeStreamOperators<HistoryEntry> ("LeechCraft::Plugins::HistoryHolder::Core::HistoryEntry");

	QSettings settings (Proxy::Instance ()->GetOrganizationName (),
			Proxy::Instance ()->GetApplicationName () + "_HistoryHolder");
	int size = settings.beginReadArray ("History");
	for (int i = 0; i < size; ++i)
	{
		settings.setArrayIndex (i);
		History_.append (settings.value ("Item").value<HistoryEntry> ());
	}
	settings.endArray ();

	QAction *remove = ToolBar_->addAction (tr ("Remove"),
			this,
			SLOT (remove ()));
	remove->setProperty ("ActionIcon", "remove");
}

Core& Core::Instance ()
{
	static Core core;
	return core;
}

void Core::Release ()
{
	ToolBar_.reset ();
}

void Core::SetCoreProxy (ICoreProxy_ptr proxy)
{
	CoreProxy_ = proxy;
}

ICoreProxy_ptr Core::GetCoreProxy () const
{
	return CoreProxy_;
}

void Core::Handle (const LeechCraft::DownloadEntity& entity)
{
	if (entity.Parameters_ & LeechCraft::DoNotSaveInHistory ||
			entity.Parameters_ & LeechCraft::IsntDownloaded)
		return;

	HistoryEntry entry =
	{
		entity,
		QDateTime::currentDateTime ()
	};

	beginInsertRows (QModelIndex (), 0, 0);
	History_.prepend (entry);
	endInsertRows ();

	WriteSettings ();
}

int Core::columnCount (const QModelIndex&) const
{
	return Headers_.size ();
}

QVariant Core::data (const QModelIndex& index, int role) const
{
	int row = index.row ();
	if (role == Qt::DisplayRole)
	{
		switch (index.column ())
		{
			case 0:
				{
					HistoryEntry e = History_.at (row);
					QString entity = e.Entity_.Location_;

					if (e.Entity_.Entity_.size () < 150)
					{
						entity += " (";
						entity += QTextCodec::codecForName ("UTF-8")->
							toUnicode (e.Entity_.Entity_);
						entity += ")";
					}
					return entity;
				}
			case 1:
				return History_.at (row).DateTime_;
			case 2:
				return data (index, RoleTags).toStringList ().join ("; ");
			default:
				return QVariant ();
		}
	}
	else if (role == RoleTags)
//		TODO
//		return History_.at (row).Entity_
		return QStringList ();
	else if (role == RoleControls)
		return QVariant::fromValue<QToolBar*> (ToolBar_.get ());
	else if (role == RoleHash)
		return History_.at (row).Entity_.Entity_;
	else if (role == RoleMime)
		return History_.at (row).Entity_.Mime_;
	else
		return QVariant ();
}

QVariant Core::headerData (int section, Qt::Orientation orient, int role) const
{
	if (orient != Qt::Horizontal ||
			role != Qt::DisplayRole)
		return QVariant ();

	return Headers_ [section];
}

QModelIndex Core::index (int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid () ||
			!hasIndex (row, column, parent))
		return QModelIndex ();

	return createIndex (row, column);
}

QModelIndex Core::parent (const QModelIndex&) const
{
	return QModelIndex ();
}

int Core::rowCount (const QModelIndex& index) const
{
	return index.isValid () ? 0 : History_.size ();
}

void Core::WriteSettings ()
{
	QSettings settings (Proxy::Instance ()->GetOrganizationName (),
			Proxy::Instance ()->GetApplicationName () + "_HistoryHolder");
	settings.beginWriteArray ("History");
	settings.remove ("");
	int i = 0;
	Q_FOREACH (HistoryEntry e, History_)
	{
		settings.setArrayIndex (i++);
		settings.setValue ("Item", QVariant::fromValue<HistoryEntry> (e));
	}
	settings.endArray ();
}

void Core::remove ()
{
	QModelIndex index = CoreProxy_->GetMainView ()->
		selectionModel ()->currentIndex ();
	index = CoreProxy_->MapToSource (index);
	if (!index.isValid ())
	{
		qWarning () << Q_FUNC_INFO
			<< "invalid index"
			<< index;
		return;
	}
	
	const FindProxy *sm = qobject_cast<const FindProxy*> (index.model ());
	index = sm->mapToSource (index);

	beginRemoveRows (QModelIndex (), index.row (), index.row ());
	History_.removeAt (index.row ());
	endRemoveRows ();

	WriteSettings ();
}

