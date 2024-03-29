#include "FeedNews.hpp"
#include "PlatformMacro.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/Version.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"
#include "InternetUpdater.hpp"
#include "Ultimate.hpp"

#include <QNetworkRequest>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>

FeedNews *FeedNews::feedNews=NULL;

FeedNews::FeedNews()
{
    qRegisterMetaType<std::vector<FeedNews::FeedEntry> >("std::vector<FeedNews::FeedEntry>");
    setObjectName("FeedNews");
    qnam=NULL;
    if(!connect(&newUpdateTimer,&QTimer::timeout,this,&FeedNews::downloadFile))
        abort();
    if(!connect(&firstUpdateTimer,&QTimer::timeout,this,&FeedNews::downloadFile))
        abort();
    newUpdateTimer.start(60*60*1000);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(5);
}

FeedNews::~FeedNews()
{
    if(qnam!=NULL)
        delete qnam;
}

void FeedNews::downloadFile()
{
    #ifndef __EMSCRIPTEN__
    QString catchChallengerVersion;
    if(Ultimate::ultimate.isUltimate())
        catchChallengerVersion=QStringLiteral("CatchChallenger Ultimate/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
    else
        catchChallengerVersion=QStringLiteral("CatchChallenger/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
    #if defined(_WIN32) || defined(Q_OS_MAC)
    catchChallengerVersion+=QStringLiteral(" (OS: %1)").arg(QString::fromStdString(InternetUpdater::GetOSDisplayString()));
    #endif
    #endif
    if(qnam==NULL)
        qnam=new QNetworkAccessManager(this);
    QNetworkRequest networkRequest(QStringLiteral(CATCHCHALLENGER_RSS_URL));
    #ifndef __EMSCRIPTEN__
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,catchChallengerVersion);
    #endif
    reply = qnam->get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &FeedNews::httpFinished);
}

void FeedNews::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        qDebug() << (QStringLiteral("get the new update failed: not finished"));
        reply->deleteLater();
        return;
    }
    else if (reply->error())
    {
        emit feedEntryList(std::vector<FeedEntry>(),reply->errorString().toStdString());
        qDebug() << (QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    else if (!redirectionTarget.isNull())
    {
        emit feedEntryList(std::vector<FeedEntry>(),tr("Redirection denied to").toStdString());
        qDebug() << (QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    loadFeeds(reply->readAll());
}

void FeedNews::loadFeeds(const QByteArray &data)
{
    //open and quick check the file
    tinyxml2::XMLDocument domDocument;
    if(domDocument.Parse(data.data(),data.size())!=0)
        return;
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
        return;

    if(root->Name()==NULL)
        emit feedEntryList(std::vector<FeedEntry>(),tr("Not Rss or Atom feed").toStdString());
    else if(strcmp(root->Name(),"rss")==0)
        loadRss(root);
    else if(strcmp(root->Name(),"feed")==0)
        loadAtom(root);
    else
        emit feedEntryList(std::vector<FeedEntry>(),tr("Not Rss or Atom feed").toStdString());
}

void FeedNews::loadRss(const tinyxml2::XMLElement *root)
{
    std::vector<FeedEntry> entryList;
    //load the content
    const tinyxml2::XMLElement *channelItem = root->FirstChildElement("channel");
    if(channelItem!=NULL)
    {
        const tinyxml2::XMLElement *item = channelItem->FirstChildElement("item");
        while(item!=NULL)
        {
            std::string description,title,link;
            QString pubDate;
            {
                const tinyxml2::XMLElement *descriptionItem = item->FirstChildElement("description");
                if(descriptionItem!=NULL)
                    description=descriptionItem->GetText();
            }
            {
                const tinyxml2::XMLElement *titleItem = item->FirstChildElement("title");
                if(titleItem!=NULL)
                    title=titleItem->GetText();
            }
            {
                const tinyxml2::XMLElement *linkItem = item->FirstChildElement("link");
                if(linkItem!=NULL)
                    link=linkItem->GetText();
            }
            {
                const tinyxml2::XMLElement *pubDateItem = item->FirstChildElement("pubDate");
                if(pubDateItem!=NULL)
                    pubDate=QString::fromStdString(pubDateItem->GetText());
            }
            pubDate = pubDate.remove(QStringLiteral(" GMT"), Qt::CaseInsensitive);
            pubDate = pubDate.remove(QRegularExpression(QStringLiteral("\\+0[0-9]{4}$")));
            QDateTime date=QDateTime::fromString(pubDate,"ddd, dd MMM yyyy hh:mm:ss");
            if(!date.isValid())
                pubDate.clear();
            FeedEntry rssEntry;
            rssEntry.description=description;
            rssEntry.title=title;
            rssEntry.pubDate=date;
            rssEntry.link=link;
            entryList.push_back(rssEntry);
            item = item->NextSiblingElement("item");
        }
    }
    emit feedEntryList(entryList);
}

void FeedNews::loadAtom(const tinyxml2::XMLElement *root)
{
    std::vector<FeedEntry> entryList;
    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("entry");
    while(item!=NULL)
    {
        std::string description,title,link;
        QString pubDate;
        {
            const tinyxml2::XMLElement *descriptionItem = item->FirstChildElement("content");
            if(descriptionItem!=NULL)
                description=descriptionItem->GetText();
        }
        {
            const tinyxml2::XMLElement *titleItem = item->FirstChildElement("title");
            if(titleItem!=NULL)
                title=titleItem->GetText();
        }
        {
            const tinyxml2::XMLElement *linkItem = item->FirstChildElement("link");
            if(linkItem!=NULL && linkItem->Attribute("href")!=NULL)
                link=linkItem->Attribute("href");
        }
        {
            const tinyxml2::XMLElement *pubDateItem = item->FirstChildElement("published");
            if(pubDateItem!=NULL)
                pubDate=QString::fromStdString(pubDateItem->GetText());
        }
        pubDate = pubDate.remove(QStringLiteral(" GMT"), Qt::CaseInsensitive);
        pubDate = pubDate.remove(QRegularExpression(QStringLiteral("\\+0[0-9]{4}$")));
        QDateTime date=QDateTime::fromString(pubDate,"yyyy-MMM-ddThh:mm:ss");
        if(!date.isValid())
            pubDate.clear();
        FeedEntry rssEntry;
        rssEntry.description=description;
        rssEntry.title=title;
        rssEntry.pubDate=date;
        rssEntry.link=link;
        entryList.push_back(rssEntry);
        item = item->NextSiblingElement("entry");
    }
    emit feedEntryList(entryList);
}
