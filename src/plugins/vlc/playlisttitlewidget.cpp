/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include "playlisttitlewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QLabel>

namespace LeechCraft
{
namespace vlc
{
	PlaylistTitleWidget::PlaylistTitleWidget (QWidget *parent)
	: QWidget (parent)
	{
		QVBoxLayout *layout = new QVBoxLayout;
		layout->addWidget (new QLabel (tr ("Playlist")));
		QWidget *widget = new QWidget;
		
		QHBoxLayout *layout2 = new QHBoxLayout;
		
		ClearPlaylist_ = new QToolButton;
		ClearAction_ = new QAction (ClearPlaylist_);
		ClearPlaylist_->setDefaultAction (ClearAction_);
		
		MagicSort_ = new QToolButton;
		MagicAction_ = new QAction (MagicSort_);
		MagicSort_->setDefaultAction (MagicAction_);
		
		AddFiles_ = new QToolButton;
		AddAction_ = new QAction (AddFiles_);
		AddFiles_->setDefaultAction (AddAction_);
		
		layout2->addWidget (AddFiles_);
		layout2->addWidget (MagicSort_);
		layout2->addWidget (ClearPlaylist_);
	
		widget->setLayout (layout2);		
		layout->addWidget (widget);
		setLayout (layout);
	}
}
}
