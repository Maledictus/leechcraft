#include <QMessageBox>
#include <QtDebug>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QCompleter>
#include <QSystemTrayIcon>
#include <QPainter>
#include <QMenu>
#include <QToolBar>
#include <QtWebKit>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <plugininterface/tagscompletionmodel.h>
#include <plugininterface/tagscompleter.h>
#include <plugininterface/util.h>
#include "aggregator.h"
#include "core.h"
#include "addfeed.h"
#include "itemsfiltermodel.h"
#include "channelsfiltermodel.h"
#include "xmlsettingsmanager.h"
#include "itembucket.h"
#include "regexpmatcherui.h"
#include "regexpmatchermanager.h"
#include "importopml.h"
#include "exportopml.h"
#include "itembucket.h"

Aggregator::~Aggregator ()
{
}

void Aggregator::Init ()
{
	Translator_.reset (LeechCraft::Util::InstallTranslator ("aggregator"));
	SetupMenuBar ();
    Ui_.setupUi (this);

    ItemBucket_.reset (new ItemBucket (this));
	dynamic_cast<QVBoxLayout*> (layout ())->insertWidget (0, ToolBar_);

    TrayIcon_.reset (new QSystemTrayIcon (this));
    TrayIcon_->hide ();
    connect (TrayIcon_.get (),
			SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
			this,
			SLOT (trayIconActivated ()));

	connect (&Core::Instance (),
			SIGNAL (error (const QString&)),
			this,
			SLOT (showError (const QString&)));
	connect (&Core::Instance (),
			SIGNAL (showDownloadMessage (const QString&)),
			this,
			SIGNAL (downloadFinished (const QString&)));
	connect (&Core::Instance (),
			SIGNAL (unreadNumberChanged (int)),
			this,
			SLOT (unreadNumberChanged (int)));

    XmlSettingsDialog_.reset (new XmlSettingsDialog (this));
    XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (), ":/aggregatorsettings.xml");

    Core::Instance ().DoDelayedInit ();
    ItemsFilterModel_.reset (new ItemsFilterModel (this));
    ItemsFilterModel_->setSourceModel (&Core::Instance ());
    ItemsFilterModel_->setFilterKeyColumn (0);
    ItemsFilterModel_->setDynamicSortFilter (true);
    ItemsFilterModel_->setFilterCaseSensitivity (Qt::CaseInsensitive);
    Ui_.Items_->setModel (ItemsFilterModel_.get ());
    Ui_.Items_->addAction (ActionMarkItemAsUnread_);
	Ui_.Items_->addAction (ActionAddToItemBucket_);
    Ui_.Items_->setContextMenuPolicy (Qt::ActionsContextMenu);
    connect (Ui_.SearchLine_,
			SIGNAL (textChanged (const QString&)),
			this,
			SLOT (updateItemsFilter ()));
    connect (Ui_.SearchType_,
			SIGNAL (currentIndexChanged (int)),
			this,
			SLOT (updateItemsFilter ()));
	QHeaderView *itemsHeader = Ui_.Items_->header ();
	QFontMetrics fm = fontMetrics ();
	int dateTimeSize = fm.width (QDateTime::currentDateTime ().toString (Qt::SystemLocaleShortDate)) + fm.width("__");
	itemsHeader->resizeSection (0,
			fm.width ("Average news article size is about this width or maybe bigger, because they are bigger"));
	itemsHeader->resizeSection (1,
			dateTimeSize);
	connect (Ui_.Items_->header (),
			SIGNAL (sectionClicked (int)),
			this,
			SLOT (makeCurrentItemVisible ()));
    ChannelsFilterModel_.reset (new ChannelsFilterModel (this));
    ChannelsFilterModel_->setSourceModel (Core::Instance ().GetChannelsModel ());
    ChannelsFilterModel_->setFilterKeyColumn (0);
    ChannelsFilterModel_->setDynamicSortFilter (true);
    Ui_.Feeds_->setModel (ChannelsFilterModel_.get ());
    Ui_.Feeds_->addAction (ActionMarkChannelAsRead_);
    Ui_.Feeds_->addAction (ActionMarkChannelAsUnread_);
    Ui_.Feeds_->setContextMenuPolicy (Qt::ActionsContextMenu);
    QHeaderView *channelsHeader = Ui_.Feeds_->header ();
    channelsHeader->resizeSection (0, fm.width ("Average channel name"));
    channelsHeader->resizeSection (1, dateTimeSize);
    channelsHeader->resizeSection (2, fm.width ("_999_"));
    connect (Ui_.TagsLine_,
			SIGNAL (textChanged (const QString&)),
			ChannelsFilterModel_.get (),
			SLOT (setFilterFixedString (const QString&)));
    connect (Ui_.Feeds_->selectionModel (),
			SIGNAL (currentChanged (const QModelIndex&, const QModelIndex&)),
			this,
			SLOT (currentChannelChanged ()));
	connect (Ui_.Items_->selectionModel (),
			SIGNAL (selectionChanged (const QItemSelection&, const QItemSelection&)),
			this,
			SLOT (currentItemChanged (const QItemSelection&)));
    connect (ActionUpdateFeeds_,
			SIGNAL (triggered ()),
			&Core::Instance (),
			SLOT (updateFeeds ()));

    TagsLineCompleter_.reset (new TagsCompleter (Ui_.TagsLine_));
    ChannelTagsCompleter_.reset (new TagsCompleter (Ui_.ChannelTags_));
    TagsLineCompleter_->setModel (Core::Instance ().GetTagsCompletionModel ());
    ChannelTagsCompleter_->setModel (Core::Instance ().GetTagsCompletionModel ());

	Ui_.TagsLine_->AddSelector ();

	Ui_.ChannelSplitter_->setStretchFactor (0, 5);
	Ui_.ChannelSplitter_->setStretchFactor (1, 2);
    Ui_.MainSplitter_->setStretchFactor (0, 5);
    Ui_.MainSplitter_->setStretchFactor (1, 9);

	connect (&RegexpMatcherManager::Instance (),
			SIGNAL (gotLink (const QString&)),
			this,
			SIGNAL (fileDownloaded (const QString&)));
	connect (Ui_.MainSplitter_,
			SIGNAL (splitterMoved (int, int)),
			this,
			SLOT (updatePixmap (int)));

	XmlSettingsManager::Instance ()->RegisterObject ("StandardFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("FixedFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("SerifFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("SansSerifFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("CursiveFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("FantasyFont", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("MinimumFontSize", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("DefaultFontSize", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("DefaultFixedFontSize", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("AutoLoadImages", this, "viewerSettingsChanged");
	XmlSettingsManager::Instance ()->RegisterObject ("AllowJavaScript", this, "viewerSettingsChanged");

	viewerSettingsChanged ();
}

void Aggregator::Release ()
{
    Core::Instance ().Release ();
    TrayIcon_->hide ();
}

QString Aggregator::GetName () const
{
    return "Aggregator";
}

QString Aggregator::GetInfo () const
{
    return tr ("RSS 2.0, Atom 1.0 feed reader.");
}

QStringList Aggregator::Provides () const
{
    return QStringList ("rss");
}

QStringList Aggregator::Needs () const
{
    return QStringList ("http");
}

QStringList Aggregator::Uses () const
{
    return QStringList ();
}

void Aggregator::SetProvider (QObject* object, const QString& feature)
{
    Core::Instance ().SetProvider (object, feature);
}

QIcon Aggregator::GetIcon () const
{
    return windowIcon ();
}

QWidget* Aggregator::GetTabContents ()
{
	return this;
}

void Aggregator::SetupMenuBar ()
{
	ToolBar_ = new QToolBar (this);

	ActionAddFeed_ = new QAction (tr ("Add feed..."),
			this);
	ActionAddFeed_->setObjectName ("ActionAddFeed_");
	ActionAddFeed_->setProperty ("ActionIcon", "aggregator_add");

	ActionPreferences_ = new QAction (tr ("Preferences..."),
			this);
	ActionPreferences_->setObjectName ("ActionPreferences_");
	ActionPreferences_->setProperty ("ActionIcon", "aggregator_preferences");

	ActionUpdateFeeds_ = new QAction (tr ("Update all feeds"),
			this);
	ActionUpdateFeeds_->setProperty ("ActionIcon", "aggregator_updateallfeeds");

	ActionRemoveFeed_ = new QAction (tr ("Remove feed"),
			this);
	ActionRemoveFeed_->setObjectName ("ActionRemoveFeed_");
	ActionRemoveFeed_->setProperty ("ActionIcon", "aggregator_remove");

	ActionMarkItemAsUnread_ = new QAction (tr ("Mark item as unread"),
			this);
	ActionMarkItemAsUnread_->setObjectName ("ActionMarkItemAsUnread_");

	ActionMarkChannelAsRead_ = new QAction (tr ("Mark channel as read"),
			this);
	ActionMarkChannelAsRead_->setObjectName ("ActionMarkChannelAsRead_");

	ActionMarkChannelAsUnread_ = new QAction (tr ("Mark channel as unread"),
			this);
	ActionMarkChannelAsUnread_->setObjectName ("ActionMarkChannelAsUnread_");

	ActionUpdateSelectedFeed_ = new QAction (tr ("Update selected feed"),
			this);
	ActionUpdateSelectedFeed_->setObjectName ("ActionUpdateSelectedFeed_");
	ActionUpdateSelectedFeed_->setProperty ("ActionIcon", "aggregator_updateselectedfeed");

	ActionAddToItemBucket_ = new QAction (tr ("Add to item bucket"),
			this);
	ActionAddToItemBucket_->setObjectName ("ActionAddToItemBucket_");

	ActionItemBucket_ = new QAction (tr ("Item bucket..."),
			this);
	ActionItemBucket_->setObjectName ("ActionItemBucket_");
	ActionItemBucket_->setProperty ("ActionIcon", "aggregator_favorites");

	ActionRegexpMatcher_ = new QAction (tr ("Regexp matcher..."),
			this);
	ActionRegexpMatcher_->setObjectName ("ActionRegexpMatcher_");
	ActionRegexpMatcher_->setProperty ("ActionIcon", "aggregator_filter");

	ActionHideReadItems_ = new QAction (tr ("Hide read items"),
			this);
	ActionHideReadItems_->setObjectName ("ActionHideReadItems_");
	ActionHideReadItems_->setCheckable (true);
	ActionHideReadItems_->setProperty ("ActionIcon", "aggregator_rssshow");
	ActionHideReadItems_->setProperty ("ActionIconOff", "aggregator_rsshide");

	ActionImportOPML_ = new QAction (tr ("Import OPML..."),
			this);
	ActionImportOPML_->setObjectName ("ActionImportOPML_");
	ActionImportOPML_->setProperty ("ActionIcon", "aggregator_importopml");

	ActionExportOPML_ = new QAction (tr ("Export OPML..."),
			this);
	ActionExportOPML_->setObjectName ("ActionExportOPML_");
	ActionExportOPML_->setProperty ("ActionIcon", "aggregator_exportopml");

    ToolBar_->addAction(ActionAddFeed_);
    ToolBar_->addAction(ActionRemoveFeed_);
    ToolBar_->addAction(ActionUpdateSelectedFeed_);
    ToolBar_->addAction(ActionUpdateFeeds_);
    ToolBar_->addSeparator();
    ToolBar_->addAction(ActionItemBucket_);
    ToolBar_->addAction(ActionRegexpMatcher_);
    ToolBar_->addSeparator();
    ToolBar_->addAction(ActionImportOPML_);
    ToolBar_->addAction(ActionExportOPML_);
    ToolBar_->addSeparator();
    ToolBar_->addAction(ActionHideReadItems_);
    ToolBar_->addSeparator();
    ToolBar_->addAction(ActionPreferences_);
}

void Aggregator::showError (const QString& msg)
{
    qWarning () << Q_FUNC_INFO << msg;
    if (!XmlSettingsManager::Instance ()->property ("BeSilent").toBool ())
        QMessageBox::warning (0, tr ("Error"), msg);
}

void Aggregator::on_ActionAddFeed__triggered ()
{
    AddFeed af;
    if (af.exec () == QDialog::Accepted)
        Core::Instance ().AddFeed (af.GetURL (), af.GetTags ());
}

void Aggregator::on_ActionRemoveFeed__triggered ()
{
	QMessageBox mb (QMessageBox::Warning,
			tr ("Warning"),
			tr ("You are going to permanently remove this feed. "
				"Are you are really sure that you want to do this?", "Feed removing confirmation"),
			QMessageBox::Ok | QMessageBox::Cancel,
			this);
	mb.setWindowModality (Qt::WindowModal);
	if (mb.exec () == QMessageBox::Ok)
		Core::Instance ().RemoveFeed (ChannelsFilterModel_->mapToSource (Ui_.Feeds_->selectionModel ()->currentIndex ()));
}

void Aggregator::on_ActionPreferences__triggered ()
{
    XmlSettingsDialog_->show ();
    XmlSettingsDialog_->setWindowTitle (windowTitle () + tr (": Preferences"));
}

void Aggregator::on_Items__activated (const QModelIndex& index)
{
	if (index.isValid ())
		Core::Instance ().Activated (ItemsFilterModel_->mapToSource (index));
}

void Aggregator::on_Feeds__activated (const QModelIndex& index)
{
	if (index.isValid ())
		Core::Instance ().FeedActivated (ChannelsFilterModel_->mapToSource (index));
}

void Aggregator::on_ActionMarkItemAsUnread__triggered ()
{
    QModelIndexList indexes = Ui_.Items_->selectionModel ()->selectedRows ();
    for (int i = 0; i < indexes.size (); ++i)
        Core::Instance ().MarkItemAsUnread (ItemsFilterModel_->mapToSource (indexes.at (i)));
}

void Aggregator::on_ActionMarkChannelAsRead__triggered ()
{
    QModelIndexList indexes = Ui_.Feeds_->selectionModel ()->selectedRows ();
    for (int i = 0; i < indexes.size (); ++i)
        Core::Instance ().MarkChannelAsRead (ChannelsFilterModel_->mapToSource (indexes.at (i)));
}

void Aggregator::on_ActionMarkChannelAsUnread__triggered ()
{
    QModelIndexList indexes = Ui_.Feeds_->selectionModel ()->selectedRows ();
    for (int i = 0; i < indexes.size (); ++i)
        Core::Instance ().MarkChannelAsUnread (ChannelsFilterModel_->mapToSource (indexes.at (i)));
}

void Aggregator::on_ActionUpdateSelectedFeed__triggered ()
{
    QModelIndex current = Ui_.Feeds_->selectionModel ()->currentIndex ();
    if (!current.isValid ())
        return;
    Core::Instance ().UpdateFeed (ChannelsFilterModel_->mapToSource (current));
}

void Aggregator::on_ChannelTags__editingFinished ()
{
    QString tags = Ui_.ChannelTags_->text ();
    QModelIndex current = Ui_.Feeds_->selectionModel ()->currentIndex ();
    if (!current.isValid ())
        return;
    Core::Instance ().SetTagsForIndex (tags, ChannelsFilterModel_->mapToSource (current));
    Core::Instance ().UpdateTags (tags.split (' '));
}

void Aggregator::on_CaseSensitiveSearch__stateChanged (int state)
{
    ItemsFilterModel_->setFilterCaseSensitivity (state ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void Aggregator::on_ActionAddToItemBucket__triggered ()
{
	Core::Instance ().AddToItemBucket (ItemsFilterModel_->
			mapToSource (Ui_.Items_->selectionModel ()->
				currentIndex ()));
}

void Aggregator::on_ActionItemBucket__triggered ()
{
	ItemBucket_->show ();
}

void Aggregator::on_ActionRegexpMatcher__triggered ()
{
	RegexpMatcherUi::Instance ().show ();
}

void Aggregator::on_ActionHideReadItems__triggered ()
{
	if (ActionHideReadItems_->isChecked ())
		Ui_.Items_->selectionModel ()->reset ();
	ItemsFilterModel_->SetHideRead (ActionHideReadItems_->isChecked ());
}

void Aggregator::on_ActionImportOPML__triggered ()
{
	ImportOPML importDialog;
	if (importDialog.exec () == QDialog::Rejected)
		return;

	Core::Instance ().AddFromOPML (importDialog.GetFilename (),
			importDialog.GetTags (),
			importDialog.GetMask ());
}

void Aggregator::on_ActionExportOPML__triggered ()
{
	ExportOPML exportDialog;
	exportDialog.SetFeeds (Core::Instance ().GetFeeds ());
	if (exportDialog.exec () == QDialog::Rejected)
		return;

	Core::Instance ().ExportToOPML (exportDialog.GetDestination (),
			exportDialog.GetTitle (),
			exportDialog.GetOwner (),
			exportDialog.GetOwnerEmail (),
			exportDialog.GetSelectedFeeds ());
}

void Aggregator::on_ItemComments__linkActivated ()
{
    QModelIndex selected = Ui_.Items_->selectionModel ()->currentIndex ();
	Core::Instance ().SubscribeToComments (ItemsFilterModel_->
			mapToSource (selected));
}

void Aggregator::currentItemChanged (const QItemSelection& selection)
{
	QModelIndexList indexes = selection.indexes ();
	if (indexes.size () != 2)
		return;

	QModelIndex sindex = ItemsFilterModel_->mapToSource (indexes.at (0));
	if (!sindex.isValid ())
	{
		Ui_.ItemView_->setHtml ("");
		Ui_.ItemAuthor_->hide ();
		Ui_.ItemAuthorLabel_->hide ();
		Ui_.ItemCategory_->hide ();
		Ui_.ItemCategoryLabel_->hide ();
		Ui_.ItemLink_->setText ("");
		Ui_.ItemPubDate_->hide ();
		Ui_.ItemPubDateLabel_->hide ();
		Ui_.ItemComments_->hide ();
		Ui_.ItemCommentsLabel_->hide ();
		return;
	}

	Core::Instance ().Selected (sindex);

	Ui_.ItemView_->setHtml (Core::Instance ().GetDescription (sindex));
	connect (Ui_.ItemView_->page ()->networkAccessManager (),
			SIGNAL (sslErrors (QNetworkReply*, const QList<QSslError>&)),
			&Core::Instance (),
			SLOT (handleSslError (QNetworkReply*)));

	QString itemAuthor = Core::Instance ().GetAuthor (sindex);
	if (itemAuthor.isEmpty ())
	{
		Ui_.ItemAuthor_->hide ();
		Ui_.ItemAuthorLabel_->hide ();
	}
	else
	{
		Ui_.ItemAuthor_->setText (Core::Instance ().GetAuthor (sindex));
		Ui_.ItemAuthor_->show ();
		Ui_.ItemAuthorLabel_->show ();
	}

	QString category = Core::Instance ().GetCategory (sindex);
	if (category.isEmpty ())
	{
		Ui_.ItemCategory_->hide ();
		Ui_.ItemCategoryLabel_->hide ();
	}
	else
	{
		Ui_.ItemCategory_->setText (Core::Instance ().GetCategory (sindex));
		Ui_.ItemCategory_->show ();
		Ui_.ItemCategoryLabel_->show ();
	}

	QString link = Core::Instance ().GetLink (sindex);
	QString shortLink;
	Ui_.ItemLink_->setToolTip (link);
	if (link.size () >= 40)
		shortLink = link.left (18) + "..." + link.right (18);
	else
		shortLink = link;
	if (QUrl (link).isValid ())
	{
		link.insert (0,"<a href=\"");
		link.append ("\">" + shortLink + "</a>");
		Ui_.ItemLink_->setText (link);
		Ui_.ItemLink_->setOpenExternalLinks (true);
	}
	else
	{
		Ui_.ItemLink_->setOpenExternalLinks (false);
		Ui_.ItemLink_->setText (shortLink);
	}

	QDateTime pubDate = Core::Instance ().GetPubDate (sindex);
	if (pubDate.isValid ())
	{
		Ui_.ItemPubDate_->setDateTime (Core::Instance ().GetPubDate (sindex));
		Ui_.ItemPubDate_->show ();
		Ui_.ItemPubDateLabel_->show ();
	}
	else
	{
		Ui_.ItemPubDate_->hide ();
		Ui_.ItemPubDateLabel_->hide ();
	}

	int numComments = Core::Instance ().GetCommentsNumber (sindex);
	QString commentsRSS = Core::Instance ().GetCommentsRSS (sindex);
	if (numComments >= 0 || !commentsRSS.isEmpty ())
	{
		Ui_.ItemComments_->show ();
		Ui_.ItemCommentsLabel_->show ();

		QString text;
		if (numComments >= 0 && commentsRSS.isEmpty ())
			text = QString::number (numComments);
		else if (numComments < 0 && !commentsRSS.isEmpty ())
			text = QString ("<a href=\"#subscribe\">%1</a>").arg (tr ("Subscribe"));
		else
			text = QString ("<a href=\"#subscribe\">%1</a>").arg (numComments);

		Ui_.ItemComments_->setText (text);
	}
	else
	{
		Ui_.ItemComments_->hide ();
		Ui_.ItemCommentsLabel_->hide ();
	}
}

void Aggregator::currentChannelChanged ()
{
	Ui_.Items_->scrollToTop ();
    QModelIndex index = Ui_.Feeds_->selectionModel ()->currentIndex ();
	if (!index.isValid ())
		return;

	QModelIndex mapped = ChannelsFilterModel_->mapToSource (index);
	Core::Instance ().currentChannelChanged (mapped);
	Ui_.ChannelTags_->setText (Core::Instance ().GetTagsForIndex (mapped.row ()).join (" "));
	Core::ChannelInfo ci = Core::Instance ().GetChannelInfo (mapped);
	QString link = ci.Link_;
	QString shortLink;
	Ui_.ChannelLink_->setToolTip (link);
	if (link.size () >= 40)
		shortLink = link.left (18) + "..." + link.right (18);
	else
		shortLink = link;
	if (QUrl (link).isValid ())
	{
		link.insert (0,"<a href=\"");
		link.append ("\">" + shortLink + "</a>");
		Ui_.ChannelLink_->setText (link);
		Ui_.ChannelLink_->setOpenExternalLinks (true);
	}
	else
	{
		Ui_.ChannelLink_->setOpenExternalLinks (false);
		Ui_.ChannelLink_->setText (shortLink);
	}
	Ui_.ChannelDescription_->setHtml (ci.Description_);
	Ui_.ChannelAuthor_->setText (ci.Author_);
	Ui_.ItemView_->setHtml ("");

	updatePixmap (Ui_.MainSplitter_->sizes ().at (0));
}

void Aggregator::unreadNumberChanged (int number)
{
    if (!number || !XmlSettingsManager::Instance ()->property ("ShowIconInTray").toBool ())
    {
        TrayIcon_->hide ();
        return;
    }

    QIcon icon (":/resources/images/trayicon.png");
    QPixmap pixmap = icon.pixmap (22, 22);
    QPainter painter;
    painter.begin (&pixmap);
    QFont font = QApplication::font ();
    font.setBold (true);
    font.setPointSize (14);
    font.setFamily ("Arial");
    painter.setFont (font);
    painter.setPen (Qt::blue);
    painter.setRenderHints (QPainter::TextAntialiasing);
    painter.drawText (0, 0, 21, 21, Qt::AlignBottom | Qt::AlignRight, QString::number (number));
    painter.end ();

    TrayIcon_->setIcon (QIcon (pixmap));
    TrayIcon_->show ();
}

void Aggregator::trayIconActivated ()
{
	emit bringToFront ();
	QModelIndex unread = Core::Instance ().GetUnreadChannelIndex ();
	if (unread.isValid ())
		Ui_.Feeds_->setCurrentIndex (ChannelsFilterModel_->mapFromSource (unread));
}

void Aggregator::updateItemsFilter ()
{
	int section = Ui_.SearchType_->currentIndex ();
	QString text = Ui_.SearchLine_->text ();
	switch (section)
	{
	case 1:
		ItemsFilterModel_->setFilterWildcard (text);
		break;
	case 2:
		ItemsFilterModel_->setFilterRegExp (text);
		break;
	default:
		ItemsFilterModel_->setFilterFixedString (text);
		break;
	}
}

void Aggregator::updatePixmap (int width)
{
    QModelIndex index = Ui_.Feeds_->selectionModel ()->currentIndex ();
	if (!index.isValid ())
		return;

	QModelIndex mapped = ChannelsFilterModel_->mapToSource (index);

	QPixmap pixmap = Core::Instance ().GetChannelPixmap (mapped);
	Ui_.ChannelImage_->setPixmap (pixmap.scaledToWidth (width,
				Qt::SmoothTransformation));
}

void Aggregator::viewerSettingsChanged ()
{
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::StandardFont,
			XmlSettingsManager::Instance ()->property ("StandardFont").value<QFont> ().family ());
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::FixedFont,
			XmlSettingsManager::Instance ()->property ("FixedFont").value<QFont> ().family ());
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::SerifFont,
			XmlSettingsManager::Instance ()->property ("SerifFont").value<QFont> ().family ());
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::SansSerifFont,
			XmlSettingsManager::Instance ()->property ("SansSerifFont").value<QFont> ().family ());
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::CursiveFont,
			XmlSettingsManager::Instance ()->property ("CursiveFont").value<QFont> ().family ());
	Ui_.ItemView_->settings ()->setFontFamily (QWebSettings::FantasyFont,
			XmlSettingsManager::Instance ()->property ("FantasyFont").value<QFont> ().family ());

	Ui_.ItemView_->settings ()->setFontSize (QWebSettings::MinimumFontSize,
			XmlSettingsManager::Instance ()->property ("MinimumFontSize").toInt ());
	Ui_.ItemView_->settings ()->setFontSize (QWebSettings::DefaultFontSize,
			XmlSettingsManager::Instance ()->property ("DefaultFontSize").toInt ());
	Ui_.ItemView_->settings ()->setFontSize (QWebSettings::DefaultFixedFontSize,
			XmlSettingsManager::Instance ()->property ("DefaultFixedFontSize").toInt ());
	Ui_.ItemView_->settings ()->setAttribute (QWebSettings::AutoLoadImages,
			XmlSettingsManager::Instance ()->property ("AutoLoadImages").toBool ());
	Ui_.ItemView_->settings ()->setAttribute (QWebSettings::JavascriptEnabled,
			XmlSettingsManager::Instance ()->property ("AllowJavaScript").toBool ());
}

void Aggregator::makeCurrentItemVisible ()
{
	QModelIndex item = Ui_.Items_->selectionModel ()->currentIndex ();
	if (item.isValid ())
		Ui_.Items_->scrollTo (item);
}

Q_EXPORT_PLUGIN2 (leechcraft_aggregator, Aggregator);

