#include "../base/Client.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/DatabaseFunction.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/fight/CommonFightEngine.hpp"

using namespace CatchChallenger;

void Client::loadMonsters()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_monsters_by_player_id.empty())
    {
        errorOutput("loadMonsters() Query is empty, bug");
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    //don't filter by place, dispatched in internal, market volume should be low

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_monsters_by_player_id.asyncRead(static_cast<Client *>(this),&Client::loadMonsters_static,{std::to_string(character_id)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_monsters_by_player_id.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        characterIsRightFinalStep();
        return;
    }
    else
        callbackRegistred.push(callback);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    loadMonsters_return();
    #elif CATCHCHALLENGER_DB_FILE
    loadMonsters_return();
    #else
    #error Define what do here
    #endif
}

void Client::loadMonsters_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadMonsters_return();
}

void Client::loadMonsters_return()
{
    /* No SP:
     * id(0),character(1),place(2),hp(3),monster(4),level(5),xp(6),captured_with(7),gender(8),egg_step(9),character_origin(10),buffs(11),skills(12),skills_endurance(13)
     * SP:
     * id(0),character(1),place(2),hp(3),monster(4),level(5),xp(6),captured_with(7),gender(8),egg_step(9),character_origin(10),buffs(11),skills(12),skills_endurance(13),sp(14)
     */
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint8_t &place=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
        if(!ok || (place!=0x01 && place!=0x02))
            continue;
        PlayerMonster playerMonster=loadMonsters_DatabaseReturn_to_PlayerMonster(ok);
        //finish it
        if(ok)
        {
            public_and_private_informations.encyclopedia_monster[playerMonster.monster/8]|=(1<<(7-playerMonster.monster%8));
            if(place==0x01)
                public_and_private_informations.playerMonster.push_back(playerMonster);
            else
                public_and_private_informations.warehouse_playerMonster.push_back(playerMonster);
        }
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    characterIsRightFinalStep();
}

PlayerMonster Client::loadMonsters_DatabaseReturn_to_PlayerMonster(bool &ok)
{
    ok=true;
    PlayerMonster playerMonster;
    Monster monster;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    playerMonster.id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
    if(!ok)
        std::cerr << "monsterId: " << GlobalServerData::serverPrivateVariables.db_common->value(0) << " is not a number" << std::endl;
    //stats
    if(ok)
    {
        playerMonster.hp=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
        if(ok)
        {
            /// \warning HP need be controled after the monster type and level have been loaded
        }
        else
            std::cerr << "monster hp: " << GlobalServerData::serverPrivateVariables.db_common->value(3) << " is not a number" << std::endl;
    }
    if(ok)
    {
        playerMonster.monster=GlobalServerData::serverPrivateVariables.db_common->stringtouint16(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.get_monsters().find(playerMonster.monster)==CommonDatapack::commonDatapack.get_monsters().cend())
            {
                ok=false;
                std::cerr << "monster: " << std::to_string(playerMonster.monster) << " is not into monster list" << std::endl;
            }
            else
            {
                monster=CommonDatapack::commonDatapack.get_monsters().at(playerMonster.monster);
                monster.give_sp=0;
            }
        }
        else
            std::cerr << "monster: "+GlobalServerData::serverPrivateVariables.db_common->value(4)+" is not a number" << std::endl;
    }
    if(ok)
    {
        playerMonster.level=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
        if(ok)
        {
            if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
            {
                std::cerr << "level: "+std::to_string(playerMonster.level)+" greater than "+std::to_string(CATCHCHALLENGER_MONSTER_LEVEL_MAX)+", truncated" << std::endl;
                playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            }
        }
        else
            std::cerr << "level: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" is not a number" << std::endl;
    }
    if(ok)
    {
        playerMonster.remaining_xp=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(6),&ok);
        if(ok)
        {
            if(playerMonster.level>monster.level_to_xp.size())
            {
                std::cerr << "monster level: "+std::to_string(playerMonster.level)+" greater than loaded level "+std::to_string(monster.level_to_xp.size()) << std::endl;
                ok=false;
            }
            else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
            {
                std::cerr << "monster xp: "+std::to_string(playerMonster.remaining_xp)+" greater than "+std::to_string(monster.level_to_xp.at(playerMonster.level-1))+", truncated" << std::endl;
                playerMonster.remaining_xp=0;
            }
        }
        else
            std::cerr << "monster xp: "+GlobalServerData::serverPrivateVariables.db_common->value(6)+" is not a number" << std::endl;
    }
    if(ok)
    {
        playerMonster.catched_with=GlobalServerData::serverPrivateVariables.db_common->stringtouint16(GlobalServerData::serverPrivateVariables.db_common->value(7),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.get_items().item.find(playerMonster.catched_with)==CommonDatapack::commonDatapack.get_items().item.cend())
                std::cerr << "captured_with: "+std::to_string(playerMonster.catched_with)+" is not is not into items list" << std::endl;
        }
        else
            std::cerr << "captured_with: "+GlobalServerData::serverPrivateVariables.db_common->value(7)+" is not a number" << std::endl;
    }
    if(ok)
    {
        const uint32_t &genderInt=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(8),&ok);
        if(ok)
        {
            if(genderInt>=1 && genderInt<=3)
                playerMonster.gender=static_cast<Gender>(genderInt);
            else
            {
                playerMonster.gender=Gender_Unknown;
                std::cerr << "unknown monster gender, out of range: "+GlobalServerData::serverPrivateVariables.db_common->value(8) << std::endl;
                ok=false;
            }
        }
        else
        {
            playerMonster.gender=Gender_Unknown;
            std::cerr << "unknown monster gender: "+GlobalServerData::serverPrivateVariables.db_common->value(8) << std::endl;
            ok=false;
        }
    }
    if(ok)
    {
        playerMonster.egg_step=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(9),&ok);
        if(!ok)
            std::cerr << "monster egg_step: "+GlobalServerData::serverPrivateVariables.db_common->value(9)+" is not a number" << std::endl;
    }
    if(ok)
    {
        playerMonster.character_origin=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(10),&ok);
        if(!ok)
            std::cerr << "monster character_origin: "+GlobalServerData::serverPrivateVariables.db_common->value(10)+" is not a number" << std::endl;
    }
    //buffs
    if(ok)
        /*ok=ignore if just buff is buggy*/loadBuffBlock(GlobalServerData::serverPrivateVariables.db_common->value(11),playerMonster);
    //skills
    if(ok)
        ok=loadSkillBlock(GlobalServerData::serverPrivateVariables.db_common->value(12),playerMonster);
    //skills_endurance
    if(ok)
        ok=loadSkillEnduranceBlock(GlobalServerData::serverPrivateVariables.db_common->value(13),playerMonster);
    if(ok)
    {
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            playerMonster.sp=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(14),&ok);
            if(!ok)
                std::cerr << "monster sp: "+GlobalServerData::serverPrivateVariables.db_common->value(14)+" is not a number" << std::endl;
        }
        else
            playerMonster.sp=0;
    }
    if(ok)
    {
        const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
        if(playerMonster.hp>stat.hp)
        {
            std::cerr << "monster hp: "+std::to_string(playerMonster.hp)+
                         " greater than max hp "+std::to_string(stat.hp)+
                         " for the level "+std::to_string(playerMonster.level)+
                         " of the monster database id: "+std::to_string(playerMonster.id)+
                         ", truncated" << std::endl;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #endif
            playerMonster.hp=stat.hp;
        }
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    playerMonster.catched_with=0;
    playerMonster.character_origin=0;
    playerMonster.egg_step=0;
    playerMonster.gender=Gender_Unknown;
    playerMonster.hp=0;
    playerMonster.id=0;
    playerMonster.level=0;
    playerMonster.monster=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    #elif CATCHCHALLENGER_DB_FILE
    playerMonster.catched_with=0;
    playerMonster.character_origin=0;
    playerMonster.egg_step=0;
    playerMonster.gender=Gender_Unknown;
    playerMonster.hp=0;
    playerMonster.id=0;
    playerMonster.level=0;
    playerMonster.monster=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    #else
    #error Define what do here
    #endif
    return playerMonster;
}

bool Client::loadBuffBlock(const std::string &dataHexa,PlayerMonster &playerMonster)
{
    if(dataHexa.empty())
        return true;
    bool ok=true;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const std::vector<char> &buffs=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(dataHexa,&ok);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    const std::vector<char> buffs;
    #elif CATCHCHALLENGER_DB_FILE
    const std::vector<char> buffs;
    #else
    #error Define what do here
    #endif
    #ifndef CATCHCHALLENGER_EXTRA_CHECK
    const char * const raw_buffs=buffs.data();
    #endif
    if(!ok)
        std::cerr << "monster buffs: "+dataHexa+" is not a hexa" << std::endl;
    else
    {
        if(buffs.size()%(1+1+1)!=0)
            std::cerr << "monster buffs missing data: "+dataHexa << std::endl;
        else
        {
            playerMonster.buffs.reserve(buffs.size()/(1+1+1));
            PlayerBuff buff;
            uint16_t lastBuffId=0;
            uint32_t pos=0;
            while(pos<buffs.size())
            {
                uint16_t buffint=(uint16_t)
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        buffs
                        #else
                        raw_buffs
                        #endif
                        [pos]+lastBuffId;
                if(buffint>255)
                    buffint-=256;
                lastBuffId=buffint;
                buff.buff=static_cast<uint8_t>(buffint);
                ++pos;
                buff.level=
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        buffs
                        #else
                        raw_buffs
                        #endif
                        [pos];
                ++pos;
                buff.remainingNumberOfTurn=
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        buffs
                        #else
                        raw_buffs
                        #endif
                        [pos];
                ++pos;

                if(CommonDatapack::commonDatapack.get_monsterBuffs().find(buff.buff)==CommonDatapack::commonDatapack.get_monsterBuffs().cend())
                {
                    std::cerr << "buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(playerMonster.id)+" is not found into buff list" << std::endl;
                    ok=false;//just ignore
                }
                if(ok)
                {
                    if(buff.level>CommonDatapack::commonDatapack.get_monsterBuffs().at(buff.buff).level.size())
                    {
                        std::cerr << "buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(playerMonster.id)+" have not the level: "+std::to_string(buff.level) << std::endl;
                        ok=false;//just ignore
                    }
                }
                if(ok)
                {
                    const Buff::GeneralEffect &generalEffect=CommonDatapack::commonDatapack.get_monsterBuffs().at(buff.buff).level.at(buff.level-1);
                    if(generalEffect.duration==Buff::Duration_ThisFight)
                    {
                        std::cerr << "buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(playerMonster.id)+" can't be loaded from the db if is not permanent (generalEffect.duration==Buff::Duration_ThisFight): " << std::to_string(generalEffect.duration) << std::endl;
                        ok=false;//just ignore
                    }
                }
                if(ok)
                    playerMonster.buffs.push_back(buff);
                else
                    break;
            }
        }
    }
    return true;
}

bool Client::loadSkillBlock(const std::string &dataHexa,PlayerMonster &playerMonster)
{
    if(dataHexa.empty())
        return true;
    bool ok=true;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const std::vector<char> &skills=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(dataHexa,&ok);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    const std::vector<char> skills;
    #elif CATCHCHALLENGER_DB_FILE
    const std::vector<char> skills;
    #else
    #error Define what do here
    #endif
    const char * const raw_skills=skills.data();
    if(!ok)
        std::cerr << "monster skills: "+dataHexa+" is not a hexa" << std::endl;
    else
    {
        if(skills.size()%(1+2)!=0)
            std::cerr << "monster skills missing data: "+dataHexa << std::endl;
        else
        {
            playerMonster.skills.reserve(skills.size()/(1+2));
            PlayerMonster::PlayerSkill skill;
            uint32_t lastSkillId=0;
            uint32_t pos=0;
            while(pos<skills.size())
            {
                uint32_t skillInt=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(raw_skills+pos))+lastSkillId;
                if(skillInt>65535)
                    skillInt-=65536;
                lastSkillId=skillInt;
                skill.skill=static_cast<uint16_t>(skillInt);
                pos+=2;
                skill.level=
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        skills
                        #else
                        raw_skills
                        #endif
                        [pos];
                ++pos;
               skill.endurance=0;

               if(CommonDatapack::commonDatapack.get_monsterSkills().find(skill.skill)==CommonDatapack::commonDatapack.get_monsterSkills().cend())
               {
                   std::cerr << "skill "+std::to_string(skill.skill)+" for monsterId: "+std::to_string(playerMonster.id)+" is not found into skill list:"+binarytoHexa(skills) << std::endl;
                   return false;
               }
               if(ok)
               {
                   if(skill.level>CommonDatapack::commonDatapack.get_monsterSkills().at(skill.skill).level.size())
                   {
                       std::cerr << "skill "+std::to_string(skill.skill)+" for monsterId: "+std::to_string(playerMonster.id)+" have not the level: "+std::to_string(skill.level) << std::endl;
                       return false;
                   }
               }
               if(ok)
                   playerMonster.skills.push_back(skill);
               else
                   break;
            }
        }
    }
    return true;
}

bool Client::loadSkillEnduranceBlock(const std::string &dataHexa,PlayerMonster &playerMonster)
{
    bool ok=true;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const std::vector<char> &skills_endurance=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(dataHexa,&ok);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    const std::vector<char> skills_endurance;
    #elif CATCHCHALLENGER_DB_FILE
    const std::vector<char> skills_endurance;
    #else
    #error Define what do here
    #endif
    #ifndef CATCHCHALLENGER_EXTRA_CHECK
    const char * const raw_skills_endurance=skills_endurance.data();
    #endif
    if(!ok)
        std::cerr << "monster skills_endurance: "+dataHexa+" is not a hexa" << std::endl;
    else
    {
        if(skills_endurance.size()!=playerMonster.skills.size())
            std::cerr << "monster skills_endurance missing data: "+dataHexa << std::endl;
        else
        {
            uint32_t pos=0;
            while(pos<skills_endurance.size())
            {
                PlayerMonster::PlayerSkill &skill=playerMonster.skills[pos];
                skill.endurance=
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        skills_endurance
                        #else
                        raw_skills_endurance
                        #endif
                        [pos];
                ++pos;

                if(ok)
                {
                    if(skill.endurance>CommonDatapack::commonDatapack.get_monsterSkills().at(skill.skill).level.at(skill.level-1).endurance)
                    {
                        skill.endurance=CommonDatapack::commonDatapack.get_monsterSkills().at(skill.skill).level.at(skill.level-1).endurance;
                        std::cerr << "skill "+std::to_string(skill.skill)+" for monsterId: "+std::to_string(playerMonster.id)+" have not the endurance: "+std::to_string(skill.endurance)+": truncated" << std::endl;
                    }
                }
                if(!ok)
                    break;
            }
        }
    }
    return true;
}

void Client::generateRandomNumber()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x53;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);//set the dynamic size

    if((randomIndex+randomSize+CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE)<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        //can send the next block
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,GlobalServerData::serverPrivateVariables.randomData.data()+randomIndex+randomSize,CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);
        posOutput+=CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //need return to the first block
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,GlobalServerData::serverPrivateVariables.randomData.data(),CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);
        posOutput+=CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    randomSize+=CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE;
}

uint32_t Client::randomSeedsSize() const
{
    return randomSize;
}

uint8_t Client::getOneSeed(const uint8_t &max)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Random seed remaining: " << randomSize << std::endl;
    #endif
    const uint8_t &number=GlobalServerData::serverPrivateVariables.randomData.at(randomIndex);
    randomIndex++;
    randomSize--;
    if(randomSize<CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE)
        generateRandomNumber();
    return number%(max+1);
}
