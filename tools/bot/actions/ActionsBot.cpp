#include "ActionsAction.h"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/tinyXML2/customtinyxml2.hpp"
#include "../../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include <iostream>

void ActionsAction::preload_the_bots(const std::vector<Map_semi> &semi_loaded_map)
{
    int shops_number=0;
    int bots_number=0;
    int learnpoint_number=0;
    int healpoint_number=0;
    int marketpoint_number=0;
    int zonecapturepoint_number=0;
    int botfights_number=0;
    int botfightstigger_number=0;
    //resolv the botfights, bots, shops, learn, heal, zonecapture, market
    const int &size=semi_loaded_map.size();
    int index=0;
    bool ok;
    while(index<size)
    {
        int sub_index=0;
        const int &botssize=semi_loaded_map.at(index).old_map.bots.size();
        MapServerMini * const mapServer=static_cast<MapServerMini *>(semi_loaded_map.at(index).map);
        while(sub_index<botssize)
        {
            bots_number++;
            CatchChallenger::Map_to_send::Bot_Semi bot_Semi=semi_loaded_map.at(index).old_map.bots.at(sub_index);

            if(!stringEndsWith(bot_Semi.file,".xml"))
                bot_Semi.file+=".xml";
            loadBotFile(semi_loaded_map.at(index).map->map_file,bot_Semi.file);
            if(botFiles.find(bot_Semi.file)!=botFiles.end())
                if(botFiles.at(bot_Semi.file).find(bot_Semi.id)!=botFiles.at(bot_Semi.file).end())
                {
                    const auto &step_list=botFiles.at(bot_Semi.file).at(bot_Semi.id).step;
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    std::cout << "Bot "
                              << bot_Semi.id
                              << " ("
                              << bot_Semi.file
                              << "), spawn at: "
                              << semi_loaded_map.at(index).map->map_file
                              << " ("
                              << std::to_string(bot_Semi.point.x)
                              << ","
                              << std::to_string(bot_Semi.point.y)
                              << ")"
                              << std::endl;
                    #endif
                    auto i=step_list.begin();
                    while (i!=step_list.end())
                    {
                        const tinyxml2::XMLElement * step = i->second;
                        if(step==NULL)
                        {
                            ++i;
                            continue;
                        }

                        std::pair<uint8_t,uint8_t> pairpoint(bot_Semi.point.x,bot_Semi.point.y);

                        if(step->Attribute("type")==NULL)
                        {
                            ++i;
                            continue;
                        }
                        const std::string step_type=std::string(step->Attribute("type"));
                        if(step_type=="shop")
                        {
                            if(step->Attribute("shop")==NULL)
                                std::cerr << "Has not attribute \"shop\": for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                uint32_t shop=stringtouint32(std::string(step->Attribute("shop")),&ok);
                                if(!ok)
                                    std::cerr << "shop is not a number: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                else if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().find(shop)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_shops().end())
                                        std::cerr << "shop number is not valid shop: for bot id: "
                                                  << bot_Semi.id
                                                  << " ("
                                                  << bot_Semi.file
                                                  << "), spawn at: "
                                                  << semi_loaded_map.at(index).map->map_file
                                                  << " ("
                                                  << std::to_string(bot_Semi.point.x)
                                                  << ","
                                                  << std::to_string(bot_Semi.point.y)
                                                  << "), for step: "
                                                  << std::to_string(i->first)
                                                  << std::endl;
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "shop put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                    #endif
                                    mapServer->shops[pairpoint].push_back(shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step_type=="learn")
                        {
                            if(mapServer->learn.find(pairpoint)!=mapServer->learn.end())
                                std::cerr << "learn point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "learn point for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->learn.insert(pairpoint);
                                learnpoint_number++;
                            }
                        }
                        else if(step_type=="heal")
                        {
                            if(mapServer->heal.find(pairpoint)!=mapServer->heal.end())
                                std::cerr << "heal point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "heal point put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->heal.insert(pairpoint);
                                healpoint_number++;
                            }
                        }
                        else if(step_type=="market")
                        {
                            if(mapServer->market.find(pairpoint)!=mapServer->market.end())
                                std::cerr << "market point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "market point put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->market.insert(pairpoint);
                                marketpoint_number++;
                            }
                        }
                        else if(step_type=="zonecapture")
                        {
                            if(step->Attribute("zone")==NULL)
                                std::cerr << "zonecapture point have not the zone attribute: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else if(mapServer->zonecapture.find(pairpoint)!=mapServer->zonecapture.end())
                                    std::cerr << "zonecapture point already on the map: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                            else
                            {
                                if(step->Attribute("zone")!=nullptr)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                        std::cerr << "zonecapture point put at: for bot id: "
                                                  << bot_Semi.id
                                                  << " ("
                                                  << bot_Semi.file
                                                  << "), spawn at: "
                                                  << semi_loaded_map.at(index).map->map_file
                                                  << " ("
                                                  << std::to_string(bot_Semi.point.x)
                                                  << ","
                                                  << std::to_string(bot_Semi.point.y)
                                                  << "), for step: "
                                                  << std::to_string(i->first)
                                                  << std::endl;
                                    #endif
                                    mapServer->zonecapture[pairpoint]=QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(step->Attribute("zone")).id;
                                    zonecapturepoint_number++;
                                }
                            }
                        }
                        else if(step_type=="fight")
                        {
                            if(mapServer->botsFight.find(pairpoint)!=mapServer->botsFight.end())
                                std::cerr << "botsFight point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                uint32_t fightid=0;
                                ok=false;
                                if(step->Attribute("fightid")!=NULL)
                                    fightid=stringtouint32(std::string(step->Attribute("fightid")),&ok);
                                if(ok)
                                {
                                    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(fightid)!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().end())
                                    {
                                        if(bot_Semi.property_text.find("lookAt")!=bot_Semi.property_text.end())
                                        {
                                            CatchChallenger::Direction direction;
                                            const std::string &lookAt=bot_Semi.property_text.at("lookAt");
                                            if(lookAt=="left")
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(lookAt=="right")
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(lookAt=="top")
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(lookAt!="bottom")
                                                    std::cerr << "Wrong direction for the botp: for bot id: "
                                                              << bot_Semi.id
                                                              << " ("
                                                              << bot_Semi.file
                                                              << "), spawn at: "
                                                              << semi_loaded_map.at(index).map->map_file
                                                              << " ("
                                                              << std::to_string(bot_Semi.point.x)
                                                              << ","
                                                              << std::to_string(bot_Semi.point.y)
                                                              << "), for step: "
                                                              << std::to_string(i->first)
                                                              << std::endl;
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            std::cout << "botsFight point put at: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << std::endl;
                                            #endif
                                            mapServer->botsFight[pairpoint].push_back(fightid);
                                            botfights_number++;

                                            uint8_t fightRange=5;
                                            if(bot_Semi.property_text.find("fightRange")!=bot_Semi.property_text.end())
                                            {
                                                fightRange=stringtouint8(bot_Semi.property_text.at("fightRange"),&ok);
                                                if(!ok)
                                                {
                                                    std::cerr << "fightRange is not a number: for bot id: "
                                                              << bot_Semi.id
                                                              << " ("
                                                              << bot_Semi.file
                                                              << "), spawn at: "
                                                              << semi_loaded_map.at(index).map->map_file
                                                              << " ("
                                                              << std::to_string(bot_Semi.point.x)
                                                              << ","
                                                              << std::to_string(bot_Semi.point.y)
                                                              << "), for step: "
                                                              << std::to_string(i->first)
                                                              << std::endl;
                                                    fightRange=5;
                                                }
                                                else
                                                {
                                                    if(fightRange>10)
                                                    {
                                                        std::cerr << "fightRange is greater than 10: for bot id: "
                                                                  << bot_Semi.id
                                                                  << " ("
                                                                  << bot_Semi.file
                                                                  << "), spawn at: "
                                                                  << semi_loaded_map.at(index).map->map_file
                                                                  << " ("
                                                                  << std::to_string(bot_Semi.point.x)
                                                                  << ","
                                                                  << std::to_string(bot_Semi.point.y)
                                                                  << "), for step: "
                                                                  << std::to_string(i->first)
                                                                  << std::endl;
                                                        fightRange=5;
                                                    }
                                                }
                                            }
                                            //load the botsFightTrigger
                                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT_BOT
                                            std::cerr << "Put bot fight point: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << " in direction: "
                                                      << std::to_string(direction)
                                                      << std::endl;
                                            #endif
                                            uint8_t temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            uint8_t index_botfight_range=0;
                                            CatchChallenger::CommonMap *map=semi_loaded_map.at(index).map;
                                            CatchChallenger::CommonMap *old_map=map;
                                            while(index_botfight_range<fightRange)
                                            {
                                                if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                                    break;
                                                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                                    break;
                                                if(map!=old_map)
                                                    break;
                                                const std::pair<uint8_t,uint8_t> botFightRangePoint(temp_x,temp_y);
                                                mapServer->botsFightTrigger[botFightRangePoint].push_back(fightid);
                                                index_botfight_range++;
                                                botfightstigger_number++;
                                            }
                                        }
                                        else
                                            std::cerr << "lookAt not found: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << std::endl;
                                    }
                                    else
                                        std::cerr << "fightid not found into the list: for bot id: "
                                                  << bot_Semi.id
                                                  << " ("
                                                  << bot_Semi.file
                                                  << "), spawn at: "
                                                  << semi_loaded_map.at(index).map->map_file
                                                  << " ("
                                                  << std::to_string(bot_Semi.point.x)
                                                  << ","
                                                  << std::to_string(bot_Semi.point.y)
                                                  << "), for step: "
                                                  << std::to_string(i->first)
                                                  << std::endl;
                                }
                                else
                                    std::cerr << "botsFight point have wrong fightid: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                            }
                        }
                        ++i;
                    }
                }
            sub_index++;
        }
        index++;
    }
    botIdLoaded.clear();

    std::cout << learnpoint_number << " learn point(s) on map loaded" << std::endl;
    std::cout << zonecapturepoint_number << " zonecapture point(s) on map loaded" << std::endl;
    std::cout << healpoint_number << " heal point(s) on map loaded" << std::endl;
    std::cout << marketpoint_number << " market point(s) on map loaded" << std::endl;
    std::cout << botfights_number << " bot fight(s) on map loaded" << std::endl;
    std::cout << botfightstigger_number << " bot fights tigger(s) on map loaded" << std::endl;
    std::cout << shops_number << " shop(s) on map loaded" << std::endl;
    std::cout << bots_number << " bots(s) on map loaded" << std::endl;
}

void ActionsAction::loadBotFile(const std::string &mapfile,const std::string &file)
{
    (void)mapfile;
    if(botFiles.find(file)!=botFiles.cend())
        return;
    botFiles[file];//create the entry
    tinyxml2::XMLDocument *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    bool ok;
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
        return;
    if(strcmp(root->Name(),"bots")!=0)
    {
        std::cerr << "\"bots\" root balise not found for the xml file" << std::endl;
        return;
    }
    //load the bots
    const tinyxml2::XMLElement * child = root->FirstChildElement("bot");
    while(child!=NULL)
    {
        if(child->Attribute("id")==NULL)
            std::cerr << "Has not attribute \"id\": child->Name(): " << child->Name() << std::endl;
        else
        {
            uint32_t id=stringtouint32(child->Attribute("id"),&ok);
            if(ok)
            {
                if(botIdLoaded.find(id)!=botIdLoaded.cend())
                    std::cerr << "Bot " << id << " into file " << file << " have same id as another bot: bot->Name(): " << child->Name() << std::endl;
                botIdLoaded.insert(id);
                botFiles[file][id];
                const tinyxml2::XMLElement * step = child->FirstChildElement("step");
                while(step!=NULL)
                {
                    if(step->Attribute("id")==NULL)
                        std::cerr << "Has not attribute \"type\": bot->Name(): " << step->Name() << std::endl;
                    else if(step->Attribute("type")==NULL)
                        std::cerr << "Has not attribute \"type\": bot->Name(): " << step->Name() << std::endl;
                    else
                    {
                        uint32_t stepId=stringtouint32(step->Attribute("id"),&ok);
                        if(ok)
                            botFiles[file][id].step[stepId]=step;
                    }
                    step = step->NextSiblingElement("step");
                }
                if(botFiles.at(file).at(id).step.find(1)==botFiles.at(file).at(id).step.cend())
                    botFiles[file].erase(id);
            }
            else
                std::cerr << "Attribute \"id\" is not a number: bot->Name(): " << child->Name() << std::endl;
        }
        child = child->NextSiblingElement("bot");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    toDeleteAfterBotLoad.push_back(domDocument);
    #endif
    //delete domDocument;->reused after, need near botFiles.clear
}
