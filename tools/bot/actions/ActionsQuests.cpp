#include "ActionsAction.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/FacilityLib.hpp"

bool ActionsAction::nextStepQuest(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "drop quest step requirement for: " << quest.id;
    #endif
    if(quests.find(quest.id)==quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        std::unordered_map<uint16_t,uint32_t> items;
        items[item.item]=item.quantity;
        remove_to_inventory(api,items);
        index++;
    }
    quests[quest.id].step++;
    if(quests.at(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        quests[quest.id].step=0;
        quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(api,
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(quest.rewards.reputation.at(index).reputationId).name,
                        quest.rewards.reputation.at(index).point
                        );
            index++;
        }
        //show_reputation();
        index=0;
        while(index<quest.rewards.allow.size())
        {
            player.allow.insert(quest.rewards.allow.at(index));
            index++;
        }
    }
    return true;
}

//reputation
void ActionsAction::appendReputationPoint(CatchChallenger::Api_protocol_Qt *api,const std::string &type,const int32_t &point)
{
    if(point==0)
        return;
    const std::unordered_map<std::string,uint8_t> &datapack_reputationNameToId=QtDatapackClientLoader::datapackLoader->get_reputationNameToId();
    if(datapack_reputationNameToId.find(type)==datapack_reputationNameToId.cend())
    {
        //emit error(QStringLiteral("Unknow reputation: %1").arg(type));
        abort();
        return;
    }
    const uint16_t &reputatioId=datapack_reputationNameToId.at(type);
    CatchChallenger::PlayerReputation playerReputation;
    if(api->player_informations.reputation.find(reputatioId)!=api->player_informations.reputation.cend())
        playerReputation=api->player_informations.reputation.at(reputatioId);
    else
    {
        playerReputation.point=0;
        playerReputation.level=0;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    CatchChallenger::PlayerReputation oldPlayerReputation=playerReputation;
    int32_t old_level=playerReputation.level;
    CatchChallenger::FacilityLib::appendReputationPoint(&playerReputation,point,CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputatioId));
    if(oldPlayerReputation.level==playerReputation.level && oldPlayerReputation.point==playerReputation.point)
        return;
    if(api->player_informations.reputation.find(reputatioId)!=api->player_informations.reputation.cend())
        api->player_informations.reputation[reputatioId]=playerReputation;
    else
        api->player_informations.reputation[reputatioId]=playerReputation;
    const std::string &reputationCodeName=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputatioId).name;
    const std::unordered_map<std::string,DatapackClientLoader::ReputationExtra> &datapack_reputationExtra=QtDatapackClientLoader::datapackLoader->get_reputationExtra();
    if(old_level<playerReputation.level)
    {
        if(datapack_reputationExtra.find(reputationCodeName)!=datapack_reputationExtra.cend())
            showTip(tr("You have better reputation into %1")
                    .arg(QString::fromStdString(datapack_reputationExtra.at(reputationCodeName).name)));
        else
            showTip(tr("You have better reputation into %1")
                    .arg(QStringLiteral("???")));
    }
    else if(old_level>playerReputation.level)
    {
        if(datapack_reputationExtra.find(reputationCodeName)!=datapack_reputationExtra.cend())
            showTip(tr("You have worse reputation into %1")
                    .arg(QString::fromStdString(datapack_reputationExtra.at(reputationCodeName).name)));
        else
            showTip(tr("You have worse reputation into %1")
                    .arg(QStringLiteral("???")));
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

bool ActionsAction::botHaveQuest(const CatchChallenger::Api_protocol_Qt *api,const uint16_t &botId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    //do the not started quest here
    std::vector<CATCHCHALLENGER_TYPE_QUEST> botQuests=QtDatapackClientLoader::datapackLoader->get_botToQuestStart().at(botId);
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint32_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at ActionsAction::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(api,currentQuest))
                return true;
            else
                {}//have not the requirement
        }
        else
        {
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().find(botQuests.at(index))==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().cend())
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(api,currentQuest))
                                return true;
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    auto bots=currentQuest.steps.at(quests.at(questId).step-1).bots;
                    if(vectorcontainsAtLeastOne(bots,botId))
                        return true;//in progress
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    for( const auto& n : quests ) {
        if(!vectorcontainsAtLeastOne(botQuests,n.first) && n.second.step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(n.first);
            auto bots=currentQuest.steps.at(n.second.step-1).bots;
            if(vectorcontainsAtLeastOne(bots,botId))
                return true;//in progress, but not the starting bot
            else
                {}//it's another bot
        }
    }
    return false;
}

bool ActionsAction::tryValidateQuestStep(CatchChallenger::Api_protocol_Qt *api, const uint16_t &questId, const uint16_t &botId, bool silent)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().find(questId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().cend())
    {
        if(!silent)
            showTip(tr("Quest not found"));
        return
                false;
    }
    const CatchChallenger::Quest &quest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId);

    if(quests.find(questId)==quests.cend())
    {
        //start for the first time the quest
        if(vectorcontainsAtLeastOne(quest.steps.at(0).bots,botId)
                && haveStartQuestRequirement(api,quest))
        {
            api->startQuest(questId);
            startQuest(api,quest);
            //updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    else if(quests.at(questId).step==0)
    {
        //start again the quest if can be repeated
        if(quest.repeatable &&
                vectorcontainsAtLeastOne(quest.steps.at(0).bots,botId)
                && haveStartQuestRequirement(api,quest))
        {
            api->startQuest(questId);
            startQuest(api,quest);
            //updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    if(!haveNextStepQuestRequirements(api,quest))
    {
        if(!silent)
            showTip(tr("You don't have the requirement to continue this quest"));
        return false;
    }
    if(quests.at(questId).step>=(quest.steps.size()))
    {
        if(!silent)
            showTip(tr("You have finish the quest <b>%1</b>").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId).name)));
        api->finishQuest(questId);
        nextStepQuest(api,quest);
        //updateDisplayedQuests();
        return true;
    }
    if(vectorcontainsAtLeastOne(quest.steps.at(quests.at(questId).step).bots,botId))
    {
        if(!silent)
            showTip(tr("You need talk to another bot"));
        return false;
    }
    api->nextQuestStep(questId);
    nextStepQuest(api,quest);
    //updateDisplayedQuests();
    return true;
}

bool ActionsAction::haveNextStepQuestRequirements(const CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    if(quests.find(quest.id)==quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1, step: %2").arg(questId).arg(step);
    #endif
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(itemQuantity(api,item.item)<item.quantity)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not the quantity for the item: " << item.item;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const uint32_t &fightId=requirements.fightId.at(index);
        if(!haveBeatBot(api,fightId))
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not beat the bot: " << fightId;
            #endif
            return false;
        }
        index++;
    }
    return true;
}

bool ActionsAction::haveStartQuestRequirement(const CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    const CatchChallenger::Player_private_and_public_informations &informations=api->get_player_informations_ro();
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=informations.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest requirement for: " << quest.id;
    #endif
    if(quests.find(quest.id)!=quests.cend())
    {
        if(informations.quests.at(quest.id).step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << questId;
            #endif
            return false;
        }
        if(informations.quests.at(quest.id).finish_one_time && !quest.repeatable)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "done one time and no repeatable: " << questId;
            #endif
            return false;
        }
    }
    unsigned int index=0;
    while(index<quest.requirements.quests.size())
    {
        const uint16_t &questId=quest.requirements.quests.at(index).quest;
        if(
                (quests.find(questId)==quests.cend() && !quest.requirements.quests.at(index).inverse)
                ||
                (quests.find(questId)!=quests.cend() && quest.requirements.quests.at(index).inverse)
                )
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have never started the quest: " << questId;
            #endif
            return false;
        }
        if(!informations.quests.at(questId).finish_one_time)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never finished: " << questId;
            #endif
            return false;
        }
        index++;
    }
    return haveReputationRequirements(api,quest.requirements.reputation);
}

bool ActionsAction::startQuest(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    if(quests.find(quest.id)==quests.cend())
    {
        quests[quest.id].step=1;
        quests[quest.id].finish_one_time=false;
    }
    else
        quests[quest.id].step=1;
    return true;
}

std::vector<std::pair<uint16_t, std::string> > ActionsAction::getQuestList(CatchChallenger::Api_protocol_Qt *api,const uint16_t &botId)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    std::vector<std::pair<uint16_t,std::string> > entryList;
    std::pair<CATCHCHALLENGER_TYPE_QUEST,std::string> oneEntry;
    //do the not started quest here
    std::vector<CATCHCHALLENGER_TYPE_QUEST> botQuests=QtDatapackClientLoader::datapackLoader->get_botToQuestStart().at(botId);
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint32_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at ActionsAction::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(api,currentQuest))
            {
                oneEntry.first=questId;
                if(QtDatapackClientLoader::datapackLoader->get_questsExtra().find(questId)!=QtDatapackClientLoader::datapackLoader->get_questsExtra().cend())
                    oneEntry.second=QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId).name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList.push_back(oneEntry);
            }
            else
                {}//have not the requirement
        }
        else
        {
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().find(botQuests.at(index))==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().cend())
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(api,currentQuest))
                            {
                                oneEntry.first=questId;
                                if(QtDatapackClientLoader::datapackLoader->get_questsExtra().find(questId)!=QtDatapackClientLoader::datapackLoader->get_questsExtra().cend())
                                    oneEntry.second=QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId).name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList.push_back(oneEntry);
                            }
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    auto bots=currentQuest.steps.at(quests.at(questId).step-1).bots;
                    if(vectorcontainsAtLeastOne(bots,botId))
                    {
                        oneEntry.first=questId;
                        if(QtDatapackClientLoader::datapackLoader->get_questsExtra().find(questId)!=QtDatapackClientLoader::datapackLoader->get_questsExtra().cend())
                            oneEntry.second=tr("%1 (in progress)").arg(QString::fromStdString(
                                                                           QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId).name)).toStdString();
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)").toStdString();
                        }
                        entryList.push_back(oneEntry);
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    for( const auto& n : quests ) {
        if(!vectorcontainsAtLeastOne(botQuests,n.first) && n.second.step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(n.first);
            auto bots=currentQuest.steps.at(n.second.step-1).bots;
            if(vectorcontainsAtLeastOne(bots,botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=n.first;
                oneEntry.second=tr("%1 (in progress)")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questsExtra().at(n.first).name))
                        .toStdString();
                entryList.push_back(oneEntry);
            }
            else
                {}//it's another bot
        }
    }
    return entryList;
}
