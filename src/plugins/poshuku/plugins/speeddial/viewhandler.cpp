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

#include "viewhandler.h"
#include <cmath>
#include <QHash>
#include <QtConcurrentRun>
#include <QXmlStreamWriter>
#include <QLineEdit>
#include <util/util.h>
#include <util/sll/qtutil.h>
#include <util/sll/prelude.h>
#include <util/threads/futures.h>
#include <interfaces/iwebbrowser.h>
#include <interfaces/poshuku/istoragebackend.h>
#include <interfaces/poshuku/iproxyobject.h>
#include <interfaces/poshuku/ibrowserwidget.h>
#include <interfaces/poshuku/iwebview.h>
#include "imagecache.h"
#include "xmlsettingsmanager.h"
#include "customsitesmanager.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace SpeedDial
{
	struct LoadResult
	{
		TopList_t TopPages_;
		TopList_t TopHosts_;
	};

	namespace
	{
		double GetScore (const QDateTime& then, const QDateTime& now)
		{
			return std::log (std::max<int> (then.daysTo (now) + 1, 1));
		}

		template<typename K, typename V>
		auto GetSortedVec (const QHash<K, V>& hash)
		{
#if QT_VERSION >= QT_VERSION_CHECK (5, 10, 0)
			std::vector<std::pair<K, V>> vec { hash.keyValueBegin (), hash.keyValueEnd () };
#else
			auto stlized = Util::Stlize (hash);
			std::vector<std::pair<K, V>> vec { stlized.begin (), stlized.end () };
#endif
			std::sort (vec.begin (), vec.end (), Util::Flip (Util::ComparingBy (Util::Snd)));
			return vec;
		}

		LoadResult GetTopUrls (const IStorageBackend_ptr& sb, size_t count)
		{
			history_items_t items;
			sb->LoadHistory (items);

			const auto& now = QDateTime::currentDateTime ();

			QHash<QString, double> url2score;
			QHash<QStringRef, double> host2score;
			for (const auto& item : items)
			{
				const auto score = GetScore (item.DateTime_, now);
				url2score [item.URL_] += score;

				const auto startPos = item.URL_.indexOf ("//") + 2;
				const auto endPos = item.URL_.indexOf ('/', startPos);
				if (startPos >= 0 && endPos > startPos)
					host2score [item.URL_.leftRef (endPos + 1)] += score;
			}
			const auto& hostsVec = GetSortedVec (host2score);

			TopList_t topSites;
			for (size_t i = 0; i < std::min (hostsVec.size (), count); ++i)
			{
				const auto& url = hostsVec [i].first.toString ();
				topSites.append ({ url, url });

				url2score.remove (url);
			}

			const auto& vec = GetSortedVec (url2score);

			TopList_t topPages;
			for (size_t i = 0; i < std::min (vec.size (), count); ++i)
			{
				const auto& url = vec [i].first;

				const auto& item = std::find_if (items.begin (), items.end (),
						[&url] (const HistoryItem& item) { return item.URL_ == url; });

				topPages.append ({ url, item->Title_ });
			}

			return { topPages, topSites };
		}
	}

	const size_t Rows = 2;
	const size_t Cols = 4;

	ViewHandler::ViewHandler (IBrowserWidget *browser,
			ImageCache *cache, CustomSitesManager *customManager, IProxyObject *proxy)
	: QObject { browser->GetWebView ()->GetQWidget () }
	, View_ { browser->GetWebView () }
	, BrowserWidget_ { browser }
	, ImageCache_ { cache }
	, PoshukuProxy_ { proxy }
	{
		if (XmlSettingsManager::Instance ().property ("UseStaticList").toBool ())
		{
			const auto& topList = customManager->GetTopList ();
			if (topList.isEmpty ())
			{
				deleteLater ();
				return;
			}

			WriteTables ({ { {}, topList } });
		}
		else
			LoadStatistics ();

		connect (ImageCache_,
				&ImageCache::gotSnapshot,
				this,
				[this] (const QUrl& url, const QImage& image)
				{
					const auto& elemId = QString::number (qHash (url));

					QString js;
					js += "(function() {";
					js += "var image = document.getElementById(" + elemId + ");";
					js += "if (image !== null) image.src = '" + Util::GetAsBase64Src (image) + "';";
					js += "})()";

					View_->EvaluateJS (js,
							[this] (const QVariant&)
							{
								if (!--PendingImages_)
									deleteLater ();
							});
				});
	}

	void ViewHandler::LoadStatistics ()
	{
		connect (View_->GetQWidget (),
				SIGNAL (loadStarted ()),
				this,
				SLOT (handleLoadStarted ()));

		auto proxy = PoshukuProxy_;
		Util::Sequence (this,
				QtConcurrent::run ([proxy]
				{
					const auto sb = proxy->CreateStorageBackend ();
					return GetTopUrls (sb, Rows * Cols);
				})) >>
				[this] (const LoadResult& result)
				{
					if (IsLoading_ || static_cast<size_t> (result.TopPages_.size ()) < Rows * Cols)
					{
						deleteLater ();
						return;
					}

					WriteTables ({
							{ tr ("Top pages"), result.TopPages_ },
							{ tr ("Top sites"), result.TopHosts_ }
						});
				};
	}

	void ViewHandler::handleLoadStarted ()
	{
		IsLoading_ = true;
	}

	void ViewHandler::WriteTables (const QList<QPair<QString, TopList_t>>& tables)
	{
		QString html;
		html += "<!DOCTYPE html>";

		QXmlStreamWriter w (&html);
		w.writeStartElement ("html");
		w.writeAttribute ("xmlns", "http://www.w3.org/1999/xhtml");
			w.writeStartElement ("head");
				w.writeStartElement ("meta");
					w.writeAttribute ("charset", "UTF-8");
				w.writeEndElement ();
				w.writeTextElement ("title", tr ("Speed dial"));
				w.writeTextElement ("style", R"delim(
						.centered {
							margin-left: auto;
							margin-right: auto;
						}

						.thumbimage {
							display: block;
							border: 1px solid black;
						}

						.thumbtext {
							white-space: nowrap;
							overflow: hidden;
							text-overflow: ellipsis;
							margin: 20px;
							text-align: center;
						}

						.sdlink {
							text-decoration: none;
							color: "#222";
						}
					)delim");
			w.writeEndElement ();
			w.writeStartElement ("body");
				for (const auto& table : tables)
					WriteTable (w, table.second, Rows, Cols, table.first);
			w.writeEndElement ();
		w.writeEndElement ();

		dynamic_cast<IWebWidget*> (BrowserWidget_)->SetHtml (html);
	}

	void ViewHandler::WriteTable (QXmlStreamWriter& w, const TopList_t& items,
			size_t rows, size_t cols, const QString& heading)
	{
		const auto& thumbSize = ImageCache_->GetThumbSize ();

		w.writeStartElement ("table");
		w.writeAttribute ("class", "centered");
		w.writeAttribute ("style", "margin-top: 10px");

		w.writeStartElement ("th");
		w.writeAttribute ("style", "text-align: center; font-size: 1.5em;");
		w.writeAttribute ("colspan", QString::number (cols));
		w.writeCharacters (heading);
		w.writeEndElement ();

		const auto& tdWidthStr = QString::number (thumbSize.width () + 20) + "px";

		for (size_t r = 0; r < rows; ++r)
		{
			w.writeStartElement ("tr");
			for (size_t c = 0; c < cols; ++c)
			{
				if (r * cols + c >= static_cast<size_t> (items.size ()))
					continue;

				const auto& item = items.at (r * cols + c);
				auto image = ImageCache_->GetSnapshot (item.first);
				if (image.isNull ())
				{
					image = QImage { thumbSize, QImage::Format_ARGB32 };
					image.fill (Qt::transparent);

					++PendingImages_;
				}

				w.writeStartElement ("td");
					w.writeAttribute ("style",
							QString { "max-width: %1; min-width: %1; width: %1;" }
									.arg (tdWidthStr));
					w.writeStartElement ("a");
						w.writeAttribute ("href", item.first.toEncoded ());
						w.writeAttribute ("class", "sdlink");

						w.writeStartElement ("img");
							w.writeAttribute ("src", Util::GetAsBase64Src (image));
							w.writeAttribute ("id", QString::number (qHash (QUrl { item.first })));
							w.writeAttribute ("width", QString::number (thumbSize.width ()));
							w.writeAttribute ("height", QString::number (thumbSize.height ()));
							w.writeAttribute ("class", "thumbimage centered");
						w.writeEndElement ();

						w.writeStartElement ("p");
							w.writeAttribute ("class", "thumbtext");
							w.writeCharacters (item.second);
						w.writeEndElement ();
					w.writeEndElement ();
				w.writeEndElement ();
			}
			w.writeEndElement ();
		}

		w.writeEndElement ();

		if (!PendingImages_)
			deleteLater ();
	}
}
}
}
