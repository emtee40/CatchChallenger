#include "MainScreen.hpp"
#include "../../libqtcatchchallenger/InternetUpdater.hpp"
#include "../../libqtcatchchallenger/FeedNews.hpp"
#include <iostream>
#include <QDesktopServices>
#include "../../libqtcatchchallenger/Settings.hpp"
#include "../../libqtcatchchallenger/Ultimate.hpp"
#include <QWidget>
#include "../../libqtcatchchallenger/Language.hpp"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../AudioGL.hpp"
#endif

MainScreen::MainScreen()
{
    haveUpdate=false;
    currentNewsType=0;

    /*title=new CCTitle(this);
    title->setText("Update!");*/

    solo=new CustomButton(":/CC/images/interface/button.png",this);
    multi=new CustomButton(":/CC/images/interface/button.png",this);
    options=new CustomButton(":/CC/images/interface/options.png",this);
    debug=new CustomButton(":/CC/images/interface/debug.png",this);
    facebook=new CustomButton(":/CC/images/interface/facebook.png",this);
    facebook->setOutlineColor(QColor(0,79,154));
    facebook->setPixelSize(28);
    website=new CustomButton(":/CC/images/interface/bluetoolbox.png",this);
    website->setText("w");
    website->setOutlineColor(QColor(0,79,154));
    website->setPixelSize(28);
    website->updateTextPercent(75);
    warningCanBeReset=true;

    haveFreshFeed=false;
    news=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    newsText=new CCGraphicsTextItem(news);
    newsText->setOpenExternalLinks(true);
    newsText->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    newsText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    //newsText->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption,false); not work
    updateNews();
    newsWait=new QGraphicsPixmapItem(news);
    newsWait->setPixmap(QPixmap(":/CC/images/multi/busy.png"));

    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);
    warningString=QString("<div style=\"background-color: rgb(255, 180, 180);border: 1px solid rgb(255, 221, 50);border-radius:5px;color: rgb(0, 0, 0);\">&nbsp;%1&nbsp;</div>");
    newsUpdate=new CustomButton(":/CC/images/interface/greenbutton.png",news);
    newsUpdate->setOutlineColor(QColor(44,117,0));

    #ifndef __EMSCRIPTEN__
    InternetUpdater::internetUpdater=new InternetUpdater();
    if(!connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainScreen::newUpdate))
        abort();
    #endif
    FeedNews::feedNews=new FeedNews();
    if(!connect(FeedNews::feedNews,&FeedNews::feedEntryList,this,&MainScreen::feedEntryList))
        std::cerr << "connect(RssNews::rssNews,&RssNews::rssEntryList,this,&MainWindow::rssEntryList) failed" << std::endl;
    //FeedNews::feedNews->checkCache();
    #ifndef CATCHCHALLENGER_SOLO
    solo->hide();
    #endif

    if(!connect(facebook,&CustomButton::clicked,this,&MainScreen::openFacebook))
        abort();
    if(!connect(website,&CustomButton::clicked,this,&MainScreen::openWebsite))
        abort();
    if(!connect(newsUpdate,&CustomButton::clicked,this,&MainScreen::openUpdate))
        abort();
    if(!connect(solo,&CustomButton::clicked,this,&MainScreen::goToSolo))
        abort();
    if(!connect(multi,&CustomButton::clicked,this,&MainScreen::goToMulti))
        abort();
    if(!connect(options,&CustomButton::clicked,this,&MainScreen::goToOptions))
        abort();
    if(!connect(debug,&CustomButton::clicked,this,&MainScreen::goToDebug))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&MainScreen::newLanguage,Qt::QueuedConnection))
        abort();

    #ifndef CATCHCHALLENGER_NOAUDIO
    if(!Settings::settings->contains("audioVolume"))
        Settings::settings->setValue("audioVolume",80);
    AudioGL::audio->setVolume(Settings::settings->value("audioVolume").toUInt());
    const std::string &terr=AudioGL::audio->startAmbiance(":/CC/music/loading.opus");
    if(!terr.empty())
        setError(terr);
    /*else
        setError("Sound ok: "+QAudioDeviceInfo::defaultOutputDevice().deviceName().toStdString());
    #else
        setError("Sound disabled");*/
    #endif
    newLanguage();
}

MainScreen::~MainScreen()
{
}

void MainScreen::setError(const std::string &error)
{
    if(warning->toHtml().isEmpty() || !warning->isVisible() || warningCanBeReset)
    {
        warningCanBeReset=false;
        std::cerr << "ScreenTransition::errorString(" << error << ")" << std::endl;
        warning->setVisible(true);
        warning->setHtml(warningString.arg(QString::fromStdString(error)));
    }
    if(error.empty())
    {
        warningCanBeReset=true;
        warning->setVisible(false);
    }
}

void MainScreen::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    int buttonMargin=10;
    if(widget->width()<600 || widget->height()<600)
    {
        solo->setSize(148,61);
        solo->setPixelSize(23);
        multi->setSize(148,61);
        multi->setPixelSize(23);
        options->setSize(41,46);
        options->setPixelSize(23);
        debug->setSize(41,46);
        debug->setPixelSize(23);
        facebook->setSize(41,46);
        facebook->setPixelSize(18);
        website->setSize(41,46);
        website->setPixelSize(18);
    }
    else {
        buttonMargin=30;
        solo->setSize(223,92);
        solo->setPixelSize(35);
        multi->setSize(223,92);
        multi->setPixelSize(35);
        options->setSize(62,70);
        options->setPixelSize(35);
        debug->setSize(62,70);
        debug->setPixelSize(35);
        facebook->setSize(62,70);
        facebook->setPixelSize(28);
        website->setSize(62,70);
        website->setPixelSize(28);
    }
    int verticalMargin=widget->height()/2+multi->height()/2+buttonMargin;
    if(solo->isVisible())
    {
        solo->setPos(widget->width()/2-solo->width()/2,widget->height()/2-multi->height()/2-solo->height()-buttonMargin);
        multi->setPos(widget->width()/2-multi->width()/2,widget->height()/2-multi->height()/2);
    }
    else
    {
        multi->setPos(widget->width()/2-multi->width()/2,widget->height()/2-multi->height());
        verticalMargin=widget->height()/2+buttonMargin;
    }
    debug->setPos(10,10);
    const int horizontalMargin=(multi->width()-options->width()-facebook->width()-website->width())/2;
    options->setPos(widget->width()/2-facebook->width()/2-horizontalMargin-options->width(),verticalMargin);
    facebook->setPos(widget->width()/2-facebook->width()/2,verticalMargin);
    website->setPos(widget->width()/2+facebook->width()/2+horizontalMargin,verticalMargin);
    warning->setPos(widget->width()/2-warning->boundingRect().width()/2,5);

    if(widget->height()<280)
    {
        newsUpdate->setVisible(false);
        if(currentNewsType!=0)
        {
            currentNewsType=0;
            updateNews();
        }
    }
    else {
        news->setPos(widget->width()/2-news->width()/2,widget->height()-news->height()-5);
        newsText->setPos(news->currentBorderSize()+2,news->currentBorderSize()+2);
        newsWait->setPos(news->width()-newsWait->pixmap().width()-news->currentBorderSize()-2,news->height()/2-newsWait->pixmap().height()/2);
        news->setVisible(true);

        if(widget->width()<600 || widget->height()<480)
        {
            int w=widget->width()-9*2;
            if(w>300)
                w=300;
            news->setSize(w,40);
            newsWait->setVisible(widget->width()>450 && !haveFreshFeed);
            newsUpdate->setVisible(false);

            if(currentNewsType!=1)
            {
                currentNewsType=1;
                updateNews();
            }
        }
        else {
            news->setSize(600-9*2,120);
            newsWait->setVisible(!haveFreshFeed);
            newsUpdate->setSize(136,57);
            newsUpdate->setPixelSize(18);
            newsUpdate->setPos(news->width()-136-news->currentBorderSize()-2,news->height()/2-57/2);
            newsUpdate->setVisible(haveUpdate);

            if(currentNewsType!=2)
            {
                currentNewsType=2;
                updateNews();
            }
        }
    }
}

#ifndef __EMSCRIPTEN__
void MainScreen::newUpdate(const std::string &version)
{
    Q_UNUSED(version);
    haveUpdate=true;
/*
    ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
    ui->update->setVisible(true);*/
}
#endif

void MainScreen::feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error)
{
    this->entryList=entryList;
    if(entryList.empty())
    {
        if(!error.empty())
            setError(error);
        return;
    }
    if(currentNewsType!=0)
        updateNews();

    newsWait->setVisible(false);
    haveFreshFeed=true;
}

void MainScreen::updateNews()
{
    switch(currentNewsType)
    {
        case 0:
            if(news->isVisible())
                news->setVisible(false);
        break;
        case 1:
            if(entryList.empty())
                news->setVisible(false);
            else
            {
                if(entryList.size()>1)
                    newsText->setHtml(QStringLiteral("<a href=\"%1\">%2</a>")
                              .arg(QString::fromStdString(entryList.at(0).link))
                              .arg(QString::fromStdString(entryList.at(0).title)));
                newsText->setVisible(true);
            }
        break;
        default:
        case 2:
        if(entryList.empty())
            news->setVisible(false);
        else
        {
            if(entryList.size()==1)
                newsText->setHtml(tr("Latest news:")+" "+QStringLiteral("<a href=\"%1\">%2</a>")
                                  .arg(QString::fromStdString(entryList.at(0).link))
                                  .arg(QString::fromStdString(entryList.at(0).title)));
            else
            {
                QStringList entryHtmlList;
                unsigned int index=0;
                while(index<entryList.size() && index<3)
                {
                    entryHtmlList << QStringLiteral(" - <a href=\"%1\">%2</a>")
                                     .arg(QString::fromStdString(entryList.at(index).link))
                                     .arg(QString::fromStdString(entryList.at(index).title));
                    index++;
                }
                newsText->setHtml(tr("Latest news:")+QStringLiteral("<br />")+entryHtmlList.join("<br />"));
            }
            //newsText->setStyleSheet("#news{background-color:rgb(220,220,240);border:1px solid rgb(100,150,240);border-radius:5px;color:rgb(0,0,0);}");
            newsText->setVisible(true);
        }
        break;
    }
}

void MainScreen::openWebsite()
{
    if(!QDesktopServices::openUrl(QUrl("https://catchchallenger.first-world.info/")))
        std::cerr << "MainScreen::openWebsite() failed" << std::endl;
}

void MainScreen::openFacebook()
{
    if(!QDesktopServices::openUrl(QUrl("https://www.facebook.com/CatchChallenger/")))
        std::cerr << "MainScreen::openFacebook() failed" << std::endl;
}

void MainScreen::openUpdate()
{
    if(!QDesktopServices::openUrl(QUrl("https://catchchallenger.first-world.info/download.html")))
        std::cerr << "MainScreen::openFacebook() failed" << std::endl;
}

QRectF MainScreen::boundingRect() const
{
    return QRectF();
}

void MainScreen::mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    solo->mousePressEventXY(p,pressValidated);
    multi->mousePressEventXY(p,pressValidated);
    debug->mousePressEventXY(p,pressValidated);
    options->mousePressEventXY(p,pressValidated);
    facebook->mousePressEventXY(p,pressValidated);
    website->mousePressEventXY(p,pressValidated);
    newsUpdate->mousePressEventXY(p,pressValidated);
}

void MainScreen::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    solo->mouseReleaseEventXY(p,pressValidated);
    multi->mouseReleaseEventXY(p,pressValidated);
    debug->mouseReleaseEventXY(p,pressValidated);
    options->mouseReleaseEventXY(p,pressValidated);
    facebook->mouseReleaseEventXY(p,pressValidated);
    website->mouseReleaseEventXY(p,pressValidated);
    newsUpdate->mouseReleaseEventXY(p,pressValidated);
}

void MainScreen::newLanguage()
{
    solo->setText(tr("Solo"));
    multi->setText(tr("Multi"));
    newsUpdate->setText(tr("Update!"));
    updateNews();
}
