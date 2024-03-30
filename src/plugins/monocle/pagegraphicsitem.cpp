/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "pagegraphicsitem.h"
#include <limits>
#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QMatrix>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMenu>
#include <QWidgetAction>
#include <interfaces/core/iiconthememanager.h>
#include <util/threads/futures.h>
#include "core.h"
#include "pixmapcachemanager.h"
#include "arbitraryrotationwidget.h"
#include "pageslayoutmanager.h"

namespace LC::Monocle
{
	PageGraphicsItem::PageGraphicsItem (IDocument_ptr doc, int page, QGraphicsItem *parent)
	: QGraphicsPixmapItem { parent }
	, Doc_ { std::move (doc) }
	, PageNum_ { page }
	{
		setTransformationMode (Qt::SmoothTransformation);
		setShapeMode (QGraphicsPixmapItem::BoundingRectShape);
		setPixmap (QPixmap (Doc_->GetPageSize (page)));
		setAcceptHoverEvents (true);
	}

	PageGraphicsItem::~PageGraphicsItem ()
	{
		Core::Instance ().GetPixmapCacheManager ()->PixmapDeleted (this);
	}

	void PageGraphicsItem::SetLayoutManager (PagesLayoutManager *manager)
	{
		LayoutManager_ = manager;
	}

	void PageGraphicsItem::SetReleaseHandler (std::function<void (int, QPointF)> handler)
	{
		ReleaseHandler_ = std::move (handler);
	}

	void PageGraphicsItem::SetScale (double xs, double ys)
	{
		if (std::abs (xs - XScale_) < std::numeric_limits<double>::epsilon () &&
			std::abs (ys - YScale_) < std::numeric_limits<double>::epsilon ())
			return;

		XScale_ = xs;
		YScale_ = ys;

		Invalid_ = true;

		if (ShouldRender ())
			update ();
		else
			prepareGeometryChange ();

		for (const auto& info : Item2RectInfo_)
			info.Setter_ (MapFromDoc (info.DocRect_));
	}

	int PageGraphicsItem::GetPageNum () const
	{
		return PageNum_;
	}

	QRectF PageGraphicsItem::MapFromDoc (const QRectF& rect) const
	{
		return
		{
			rect.x () * XScale_,
			rect.y () * YScale_,
			rect.width () * XScale_,
			rect.height () * YScale_
		};
	}

	QRectF PageGraphicsItem::MapToDoc (const QRectF& rect) const
	{
		return
		{
			rect.x () / XScale_,
			rect.y () / YScale_,
			rect.width () / XScale_,
			rect.height () / YScale_
		};
	}

	void PageGraphicsItem::RegisterChildRect (QGraphicsItem *item, const QRectF& srcRect, RectSetter_f setter)
	{
		const auto& pageRect = MapToDoc (boundingRect ());

		const auto& docRect = QMatrix {}.scale (pageRect.width (), pageRect.height ()).mapRect (srcRect);

		Item2RectInfo_ [item] = { docRect, std::move (setter) };
		setter (MapFromDoc (docRect));
	}

	void PageGraphicsItem::UnregisterChildRect (QGraphicsItem *item)
	{
		Item2RectInfo_.remove (item);
	}

	void PageGraphicsItem::ClearPixmap ()
	{
		setPixmap (QPixmap { QSize { 1, 1 } });

		Invalid_ = true;
	}

	void PageGraphicsItem::UpdatePixmap ()
	{
		Invalid_ = true;
		if (ShouldRender ())
			update ();
	}

	void PageGraphicsItem::paint (QPainter *painter,
			const QStyleOptionGraphicsItem *option, QWidget *w)
	{
		if (Invalid_ && IsDisplayed ())
		{
			setPixmap (GetEmptyPixmap ());

			if (ShouldRender ())
			{
				Invalid_ = false;
				Util::Sequence (this, Doc_->RenderPage (PageNum_, XScale_, YScale_)) >>
						[&, prevXScale = XScale_, prevYScale = YScale_] (const QImage& img)
						{
							setPixmap (QPixmap::fromImage (img));

							if (std::abs (prevXScale - XScale_) > std::numeric_limits<double>::epsilon () * XScale_ ||
								std::abs (prevYScale - YScale_) > std::numeric_limits<double>::epsilon () * YScale_)
								UpdatePixmap ();
							else
								Core::Instance ().GetPixmapCacheManager ()->PixmapChanged (this);
						};
			}
		}

		QGraphicsPixmapItem::paint (painter, option, w);
		Core::Instance ().GetPixmapCacheManager ()->PixmapPainted (this);
	}

	void PageGraphicsItem::mousePressEvent (QGraphicsSceneMouseEvent *event)
	{
		if (!ReleaseHandler_)
			QGraphicsItem::mousePressEvent (event);
	}

	void PageGraphicsItem::mouseReleaseEvent (QGraphicsSceneMouseEvent *event)
	{
		QGraphicsItem::mouseReleaseEvent (event);
		if (ReleaseHandler_)
			ReleaseHandler_ (PageNum_, event->pos ());
	}

	void PageGraphicsItem::contextMenuEvent (QGraphicsSceneContextMenuEvent *event)
	{
		QMenu rotateMenu;

		constexpr auto RotationStep = 90;

		auto ccwAction = rotateMenu.addAction (tr ("Rotate 90 degrees counter-clockwise"),
				this, [this] { LayoutManager_->AddRotation (-RotationStep, PageNum_); });
		ccwAction->setProperty ("ActionIcon", "object-rotate-left");

		auto cwAction = rotateMenu.addAction (tr ("Rotate 90 degrees clockwise"),
				this, [this] { LayoutManager_->AddRotation (RotationStep, PageNum_); });
		cwAction->setProperty ("ActionIcon", "object-rotate-right");

		auto arbAction = rotateMenu.addAction (tr ("Rotate arbitrarily..."));
		arbAction->setProperty ("ActionIcon", "transform-rotate");

		auto arbMenu = new QMenu ();
		arbAction->setMenu (arbMenu);

		ArbWidget_ = new ArbitraryRotationWidget;
		ArbWidget_->setValue (LayoutManager_->GetRotation () + LayoutManager_->GetRotation (PageNum_));
		connect (ArbWidget_,
				&ArbitraryRotationWidget::valueChanged,
				this,
				[this] (double rotation) { LayoutManager_->SetRotation (rotation, PageNum_); });
		auto actionWidget = new QWidgetAction (arbMenu);
		actionWidget->setDefaultWidget (ArbWidget_);
		arbMenu->addAction (actionWidget);

		Core::Instance ().GetProxy ()->GetIconThemeManager ()->ManageWidget (&rotateMenu);

		rotateMenu.exec (event->screenPos ());
	}

	QPixmap PageGraphicsItem::GetEmptyPixmap () const
	{
		QPixmap px { boundingRect ().size ().toSize () };
		px.fill ();
		return px;
	}

	bool PageGraphicsItem::IsDisplayed () const
	{
		const auto& thisMapped = mapToScene (boundingRect ()).boundingRect ();
		return std::ranges::any_of (scene ()->views (),
				[&thisMapped] (auto view)
				{
					const auto& rect = view->viewport ()->rect ();
					return view->mapToScene (rect).boundingRect ().intersects (thisMapped);
				});
	}

	void PageGraphicsItem::SetRenderingEnabled (bool enabled)
	{
		if (IsRenderingEnabled_ == enabled)
			return;

		IsRenderingEnabled_ = enabled;
		if (ShouldRender ())
			update ();
	}

	bool PageGraphicsItem::ShouldRender () const
	{
		return IsRenderingEnabled_ && IsDisplayed ();
	}

	QRectF PageGraphicsItem::boundingRect () const
	{
		QSizeF size { Doc_->GetPageSize (PageNum_) };
		size.rwidth () *= XScale_;
		size.rheight () *= YScale_;
		return QRectF { offset (), size };
	}

	QPainterPath PageGraphicsItem::shape () const
	{
		QPainterPath path;
		path.addRect (boundingRect ());
		return path;
	}
}
