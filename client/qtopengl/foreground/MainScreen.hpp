#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "../CustomButton.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CCTitle.hpp"
#include "../ScreenInput.hpp"
#include "../../libqtcatchchallenger/FeedNews.hpp"
#include "../CCGraphicsTextItem.hpp"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../../libqtcatchchallenger/QInfiniteBuffer.hpp"
#include <QAudioOutput>
#endif

class MainScreen : public ScreenInput
{
    Q_OBJECT
public:
    explicit MainScreen();
    ~MainScreen();
    void setError(const std::string &error);
    QRectF boundingRect() const override;
private:
    QGraphicsTextItem *update;
    QGraphicsTextItem *updateStar;
    QGraphicsTextItem *updateText;
    //CCTitle *title;
    CustomButton *solo;
    CustomButton *multi;
    CustomButton *options;
    CustomButton *facebook;
    CustomButton *website;
    CustomButton *debug;
    ImagesStrechMiddle *news;
    //QGraphicsTextItem *newsText;
    CCGraphicsTextItem *newsText;
    QGraphicsPixmapItem *newsWait;
    CustomButton *newsUpdate;
    QGraphicsTextItem *warning;
    bool warningCanBeReset;
    QString warningString;
    std::vector<FeedNews::FeedEntry> entryList;
    uint8_t currentNewsType;
    bool haveFreshFeed;

    bool haveUpdate;
private slots:
    void newLanguage();
protected:
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void updateNews();

    #ifndef __EMSCRIPTEN__
    void newUpdate(const std::string &version);
    #endif
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error=std::string());
    void openWebsite();
    void openFacebook();
    void openUpdate();
signals:
    void goToOptions();
    void goToDebug();
    void goToSolo();
    void goToMulti();
};

#endif // MAINSCREEN_H
