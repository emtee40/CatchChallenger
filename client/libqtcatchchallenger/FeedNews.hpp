#ifndef RSSNEWS_H
#define RSSNEWS_H

#include <vector>
#include <string>
#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "../../general/tinyXML2/tinyxml2.hpp"
#include "../../general/base/lib.h"

class DLL_PUBLIC FeedNews : public QObject
{
    Q_OBJECT
public:
    struct FeedEntry
    {
        std::string title;
        std::string link;
        std::string description;
        QDateTime pubDate;
    };
    explicit FeedNews();
    ~FeedNews();
    static FeedNews *feedNews;
    static std::string getText(const std::string &version);
signals:
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error=std::string()) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    void loadFeeds(const QByteArray &data);
    void loadRss(const tinyxml2::XMLElement *root);
    void loadAtom(const tinyxml2::XMLElement *root);
};

#endif // RSSNEWS_H
