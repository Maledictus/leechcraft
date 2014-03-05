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

#include "plotmanager.h"
#include <QStandardItemModel>
#include <QPainter>
#include <QUrl>
#include <QFile>
#include <QDir>
#include "contextwrapper.h"
#include "sensorsgraphmodel.h"

Q_DECLARE_METATYPE (QList<QPointF>)

namespace LeechCraft
{
namespace HotSensors
{
	PlotManager::PlotManager (std::weak_ptr<SensorsManager> mgr, ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, SensorsMgr_ (mgr)
	, Model_ (new SensorsGraphModel (this))
	, UpdateCounter_ (0)
	{
	}

	QAbstractItemModel* PlotManager::GetModel () const
	{
		return Model_;
	}

	QObject* PlotManager::CreateContextWrapper ()
	{
		return new ContextWrapper (this, Proxy_);
	}

	void PlotManager::handleHistoryUpdated (const ReadingsHistory_t& history)
	{
		QHash<QString, QStandardItem*> existing;
		for (int i = 0; i < Model_->rowCount (); ++i)
		{
			auto item = Model_->item (i);
			existing [item->data (SensorsGraphModel::SensorName).toString ()] = item;
		}

		QList<QStandardItem*> items;

		for (auto i = history.begin (); i != history.end (); ++i)
		{
			const auto& name = i.key ();

			QList<QPointF> points;
			for (const auto& item : *i)
				points.append ({ static_cast<qreal> (points.size ()), item.Value_ });

			const bool isKnownSensor = existing.contains (name);
			auto item = isKnownSensor ? existing.take (name) : new QStandardItem;

			const auto lastTemp = i->isEmpty () ? 0 : static_cast<int> (i->last ().Value_);
			item->setData (QString::fromUtf8 ("%1°C").arg (lastTemp), SensorsGraphModel::LastTemp);
			item->setData (name, SensorsGraphModel::SensorName);
			item->setData (QVariant::fromValue (points), SensorsGraphModel::PointsList);
			if (!isKnownSensor)
				items << item;
		}

		for (auto item : existing)
			Model_->removeRow (item->row ());

		++UpdateCounter_;

		if (!items.isEmpty ())
			Model_->invisibleRootItem ()->appendRows (items);
	}
}
}
