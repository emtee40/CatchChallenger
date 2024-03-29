#ifndef MAPDOOR_H
#define MAPDOOR_H

#include <QObject>
#include <QTimer>

#include <libtiled/mapobject.h>
#include <libtiled/tile.h>

class MapDoor : public QObject
{
    Q_OBJECT
public:
    explicit MapDoor(Tiled::MapObject* object, const uint8_t &framesCount, const uint16_t &ms);
    ~MapDoor();
    void startOpen(const uint16_t &timeRemainOpen);
    void startClose();
    uint16_t timeToOpen() const;
private:
    Tiled::MapObject* object;
    Tiled::Cell cell;
    const Tiled::Tile* baseTile;
    const uint8_t framesCount;
    const uint16_t ms;
    struct EventCall
    {
        QTimer *timer;
        uint8_t frame;//0 to framesCount*2 -> open + close
    };
    QList<EventCall> events;
private slots:
    void timerFinish();
};

#endif // MAPDOOR_H
