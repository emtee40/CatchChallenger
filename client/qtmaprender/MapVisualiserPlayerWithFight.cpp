#include "MapVisualiserPlayerWithFight.hpp"

#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/MoveOnTheMap.hpp"
#include "../qtmaprender/TemporaryTile.hpp"

#include <iostream>
#include <QDebug>

MapVisualiserPlayerWithFight::MapVisualiserPlayerWithFight(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &openGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,openGL)
{
    this->events=events;
    repel_step=0;
    items=NULL;
    quests=NULL;
    fightCollisionBot=NULL;
    botAlreadyBeaten=NULL;
}

MapVisualiserPlayerWithFight::~MapVisualiserPlayerWithFight()
{
    if(fightCollisionBot!=NULL)
    {
        //delete[] fightCollisionBot;
        fightCollisionBot=NULL;
    }
    if(botAlreadyBeaten!=NULL)
    {
        delete[] botAlreadyBeaten;
        botAlreadyBeaten=NULL;
    }
}

void MapVisualiserPlayerWithFight::setBotsAlreadyBeaten(const char * const botAlreadyBeaten)
{
    if(this->botAlreadyBeaten!=NULL)
    {
        delete[] this->botAlreadyBeaten;
        this->botAlreadyBeaten=NULL;
    }
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId())
    {
        this->botAlreadyBeaten=(char *)malloc(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
        memset(this->botAlreadyBeaten,0,CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
    }
    else
    {
        std::cerr << "MapVisualiserPlayerWithFight::setBotsAlreadyBeaten() < CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId" << std::endl;
        this->botAlreadyBeaten=NULL;
    }
    if(botAlreadyBeaten!=NULL)
        memcpy(this->botAlreadyBeaten,botAlreadyBeaten,CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
}

void MapVisualiserPlayerWithFight::addBeatenBotFight(const uint16_t &botFightId)
{
    if(botAlreadyBeaten==NULL)
        abort();
    botAlreadyBeaten[botFightId/8]|=(1<<(7-botFightId%8));
}

bool MapVisualiserPlayerWithFight::haveBeatBot(const uint16_t &botFightId) const
{
    if(botAlreadyBeaten==NULL)
        abort();
    return botAlreadyBeaten[botFightId/8] & (1<<(7-botFightId%8));
}

void MapVisualiserPlayerWithFight::addRepelStep(const uint32_t &repel_step)
{
    this->repel_step+=repel_step;
}

void MapVisualiserPlayerWithFight::resetAll()
{
    if(botAlreadyBeaten!=NULL)
    {
        delete botAlreadyBeaten;
        botAlreadyBeaten=NULL;
    }
    MapVisualiserPlayer::resetAll();
}

void MapVisualiserPlayerWithFight::keyPressParse()
{
    if(client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight at keyPressParse()";
        return;
    }

    MapVisualiserPlayer::keyPressParse();
}

bool MapVisualiserPlayerWithFight::haveStopTileAction()
{
    if(client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight at moveStepSlot()";
        return true;
    }
    if(items==NULL)
    {
        qDebug() << "items is null, can't move";
        return true;
    }
    if(events==NULL)
    {
        qDebug() << "events is null, can't move";
        return true;
    }
    CatchChallenger::PlayerMonster *fightMonster;
    if(!client->getAbleToFight())
        fightMonster=NULL;
    else
        fightMonster=client->getCurrentMonster();
    if(fightMonster!=NULL)
    {
        std::pair<uint8_t,uint8_t> pos(getPos());
        const Map_full * current_map_pointer=all_map.at(current_map);
        const std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>,pairhash> &botsFightTrigger=current_map_pointer->logicalMap.botsFightTrigger;

        if(botsFightTrigger.find(pos)!=botsFightTrigger.cend())
        {
            std::vector<uint16_t> botFightList=botsFightTrigger.at(pos);
            std::vector<std::pair<uint8_t,uint8_t> > botFightRemotePointList=
                    current_map_pointer->logicalMap.botsFightTriggerExtra.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                const uint16_t &fightId=botFightList.at(index);
                if(!haveBeatBot(fightId))
                {
                    qDebug() <<  "is now in fight with: " << fightId;
                    if(isInMove())
                    {
                        stopMove();
                        emit send_player_direction(getDirection());
                        keyPressed.clear();
                    }
                    parseStop();
                    emit botFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.at(current_map)->logicalMap),
                                           botFightRemotePointList.at(index).first,botFightRemotePointList.at(index).second);
                    if(current_map_pointer->logicalMap.botsDisplay.find(botFightRemotePointList.at(index))!=
                            current_map_pointer->logicalMap.botsDisplay.cend())
                    {
                        TemporaryTile *temporaryTile=current_map_pointer->logicalMap.botsDisplay.at(botFightRemotePointList.at(index)).temporaryTile;
                        //show a temporary flags
                        {
                            if(fightCollisionBot==NULL)
                            {
                                fightCollisionBot=Tiled::Tileset::create(QStringLiteral("fightCollisionBot"),16,16);
                                fightCollisionBot->loadFromImage(QImage(QStringLiteral(":/CC/images/fightCollisionBot.png")),QStringLiteral(":/CC/images/fightCollisionBot.png"));
                            }
                        }
                        temporaryTile->startAnimation(fightCollisionBot->tileAt(0),150,static_cast<uint8_t>(fightCollisionBot->tileCount()));
                    }
                    else
                        qDebug() <<  "temporaryTile not found";
                    blocked=true;
                    return true;
                }
                index++;
            }
        }
        //check if is in fight collision, but only if is move
        if(repel_step<=0)
        {
            if(inMove)
            {
                if(client->generateWildFightIfCollision(&current_map_pointer->logicalMap,x,y,*items,*events))
                {
                    inMove=false;
                    emit send_player_direction(direction);
                    keyPressed.clear();
                    parseStop();
                    emit wildFightCollision(static_cast<CatchChallenger::Map_client *>(&all_map.at(current_map)->logicalMap),x,y);
                    blocked=true;
                    return true;
                }
            }
        }
        else
        {
            repel_step--;
            if(repel_step==0)
                emit repelEffectIsOver();
        }
    }
    else
        qDebug() << "Strange, not monster, skip all the fight type";
    return false;
}

bool MapVisualiserPlayerWithFight::canGoTo(const CatchChallenger::Direction &direction, CatchChallenger::CommonMap map, uint8_t x, uint8_t y, const bool &checkCollision)
{
    if(!MapVisualiserPlayer::canGoTo(direction,map,x,y,checkCollision))
        return false;
    if(client->isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }
    CatchChallenger::CommonMap *new_map=&map;
    if(!CatchChallenger::MoveOnTheMap::moveWithoutTeleport(direction,&new_map,&x,&y,false))
    {
        qDebug() << "Strange, can go but move failed";
        return false;
    }
    if(all_map.find(new_map->map_file)==all_map.cend())
        return false;
    const CatchChallenger::Map_client &map_client=all_map.at(new_map->map_file)->logicalMap;

    {
        int list_size=map_client.teleport_semi.size();
        int index=0;
        while(index<list_size)
        {
            const CatchChallenger::Map_semi_teleport &teleporter=map_client.teleport_semi.at(index);
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                switch(teleporter.condition.type)
                {
                    case CatchChallenger::MapConditionType_None:
                    case CatchChallenger::MapConditionType_Clan://not do for now
                    break;
                    case CatchChallenger::MapConditionType_FightBot:
                        if(!haveBeatBot(teleporter.condition.data.fightBot))
                        {
                            if(!map_client.teleport_condition_texts.at(index).empty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    case CatchChallenger::MapConditionType_Item:
                        if(items==NULL)
                            break;
                        if(items->find(teleporter.condition.data.item)==items->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).empty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    case CatchChallenger::MapConditionType_Quest:
                        if(quests==NULL)
                            break;
                        if(quests->find(teleporter.condition.data.quest)==quests->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).empty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                        if(!quests->at(teleporter.condition.data.quest).finish_one_time)
                        {
                            if(!map_client.teleport_condition_texts.at(index).empty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                    break;
                    default:
                    break;
                }
            }
            index++;
        }
    }

    {
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(map_client.botsFightTrigger.find(pos)!=map_client.botsFightTrigger.cend())
        {
            std::vector<uint16_t> botFightList=map_client.botsFightTrigger.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                if(!haveBeatBot(botFightList.at(index)))
                {
                    if(!client->getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                        return false;
                    }
                }
                index++;
            }
        }
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*new_map,x,y);
    if(!monstersCollisionValue.walkOn.empty())
    {
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
            {
                if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                {
                    if(!client->getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
                        return false;
                    }
                    if(!client->canDoRandomFight(*new_map,x,y))
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
                        return false;
                    }
                }
                return true;
            }
            index++;
        }
        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneItem);
        return false;
    }
    return true;
}
