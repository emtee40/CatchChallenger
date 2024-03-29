#ifndef TEMPORARYTILE_H
#define TEMPORARYTILE_H

#include <QObject>
#include <QTimer>

#include <libtiled/mapobject.h>
#include <libtiled/tile.h>

class TemporaryTile : public QObject
{
    Q_OBJECT
public:
    explicit TemporaryTile(Tiled::MapObject* object);
    ~TemporaryTile();
    void startAnimation(Tiled::Tile *tile,const uint32_t &ms,const uint8_t &count);
    static Tiled::Tile *empty;
private:
    Tiled::MapObject* object;
    int index;
    uint8_t count;
    QTimer timer;
private:
    void updateTheTile();
};

#endif // TEMPORARYTILE_H
