#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include "MapVisualiserPlayer.hpp"
#include "../../general/fight/CommonFightEngine.hpp"
#include "../../general/base/lib.h"

#include <QObject>

class DLL_PUBLIC MapVisualiserPlayerWithFight : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapVisualiserPlayerWithFight(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiserPlayerWithFight();
    void setBotsAlreadyBeaten(const char * const botAlreadyBeaten);
    void addBeatenBotFight(const uint16_t &botFightId);
    bool haveBeatBot(const uint16_t &botFightId) const;
    void addRepelStep(const uint32_t &repel_step);
protected slots:
    virtual void keyPressParse();
    virtual bool haveStopTileAction();
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    virtual void resetAll();
protected:
    char * botAlreadyBeaten;
    uint32_t repel_step;
    Tiled::SharedTileset fightCollisionBot;
signals:
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const std::string &text) const;
};

#endif
