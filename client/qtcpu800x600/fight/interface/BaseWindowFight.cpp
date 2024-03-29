#include "../../base/interface/BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libcatchchallenger/ClientVariable.hpp"
#include "../../../../general/base/FacilityLib.hpp"
#include "../../../../general/base/GeneralStructures.hpp"
#include "ui_BaseWindow.h"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <iostream>

/* show only the plant into the inventory */

using namespace CatchChallenger;

void BaseWindow::on_monsterList_itemActivated(QListWidgetItem *item)
{
    if(monsterspos_items_graphical.find(item)==monsterspos_items_graphical.cend())
        return;
    uint8_t monsterPosition=monsterspos_items_graphical.at(item);
    if(inSelection)
    {
       objectSelection(true,monsterPosition);
       return;
    }
    const std::vector<PlayerMonster> &playerMonster=client->getPlayerMonster();
    unsigned int index=monsterPosition;
    const PlayerMonster &monster=playerMonster.at(index);
    const DatapackClientLoader::MonsterExtra &monsterExtraInfo=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster);
    const QtDatapackClientLoader::QtMonsterExtra &QtmonsterExtraInfo=QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster);
    const Monster &monsterGeneralInfo=CommonDatapack::commonDatapack.get_monsters().at(monster.monster);
    const Monster::Stat &stat=CommonFightEngine::getStat(monsterGeneralInfo,monster.level);
    if(monsterGeneralInfo.type.empty())
        ui->monsterDetailsType->setText(QString());
    else
    {
        QStringList typeList;
        unsigned int sub_index=0;
        while(sub_index<monsterGeneralInfo.type.size())
        {
            const auto &typeSub=monsterGeneralInfo.type.at(sub_index);
            if(QtDatapackClientLoader::datapackLoader->get_typeExtra().find(typeSub)!=QtDatapackClientLoader::datapackLoader->get_typeExtra().cend())
            {
                const DatapackClientLoader::TypeExtra &typeExtra=QtDatapackClientLoader::datapackLoader->get_typeExtra().at(typeSub);
                if(!typeExtra.name.empty())
                {
                    std::vector<char> data;
                    data.push_back(typeExtra.color.r);
                    data.push_back(typeExtra.color.g);
                    data.push_back(typeExtra.color.b);
                    //if(typeExtra.color.isValid())
                        typeList.push_back("<span style=\"background-color:#"+QString::fromStdString(binarytoHexa(data))+";\">"+QString::fromStdString(typeExtra.name)+"</span>");
                    /*else
                        typeList.push_back(QString::fromStdString(typeExtra.name));*/
                }
            }
            sub_index++;
        }
        QStringList extraInfo;
        if(!typeList.isEmpty())
            extraInfo << tr("Type: %1").arg(typeList.join(QStringLiteral(", ")));
        #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
        if(!monsterExtraInfo.kind.empty())
            extraInfo << tr("Kind: %1").arg(QString::fromStdString(monsterExtraInfo.kind));
        if(!monsterExtraInfo.habitat.empty())
            extraInfo << tr("Habitat: %1").arg(QString::fromStdString(monsterExtraInfo.habitat));
        #endif
        ui->monsterDetailsType->setText(extraInfo.join(QStringLiteral("<br />")));
    }
    ui->monsterDetailsName->setText(QString::fromStdString(monsterExtraInfo.name));
    ui->monsterDetailsDescription->setText(QString::fromStdString(monsterExtraInfo.description));
    {
        QPixmap front=QtmonsterExtraInfo.front;
        front=front.scaled(160,160);
        ui->monsterDetailsImage->setPixmap(front);
    }
    {
        QPixmap catchPixmap;
        if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(monster.catched_with)!=
                QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->getItemExtra(monster.catched_with).image;
            ui->monsterDetailsCatched->setToolTip(tr("catched with %1")
                                                  .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra()
                                                                              .at(monster.catched_with).name)));
        }
        else
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
            ui->monsterDetailsCatched->setToolTip(tr("catched with unknown item: %1").arg(monster.catched_with));
        }
        catchPixmap=catchPixmap.scaled(48,48);
        ui->monsterDetailsCatched->setPixmap(catchPixmap);
    }
    if(monster.gender==Gender_Male)
    {
        ui->monsterDetailsGender->setPixmap(QPixmap(":/images/interface/male.png").scaled(48,48));
        ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Male")));
    }
    else if(monster.gender==Gender_Female)
    {
        ui->monsterDetailsGender->setPixmap(QPixmap(":/images/interface/female.png").scaled(48,48));
        ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Female")));
    }
    else
    {
        ui->monsterDetailsGender->setPixmap(QPixmap());
        ui->monsterDetailsGender->setToolTip(QString());
    }
    const uint32_t &maxXp=monsterGeneralInfo.level_to_xp.at(monster.level-1);
    ui->monsterDetailsLevel->setText(tr("Level %1").arg(monster.level));
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    if(monster.hp>(stat.hp/2))
        ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#1A8307\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
    else if(monster.hp>(stat.hp/4))
        ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#B99C09\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
    else
        ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#BF0303\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
    #else
    ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("%1/%2").arg(monster.hp).arg(stat.hp));
    #endif
    ui->monsterDetailsStatSpeed->setText(tr("Speed: %1").arg(stat.speed));
    ui->monsterDetailsStatXp->setText(tr("Xp: %1/%2").arg(monster.remaining_xp).arg(maxXp));
    ui->monsterDetailsStatAttack->setText(tr("Attack: %1").arg(stat.attack));
    ui->monsterDetailsStatDefense->setText(tr("Defense: %1").arg(stat.defense));
    ui->monsterDetailsStatXpBar->setMaximum(maxXp);
    if(monster.remaining_xp<=maxXp)
        ui->monsterDetailsStatXpBar->setValue(monster.remaining_xp);
    else
        ui->monsterDetailsStatXpBar->setValue(maxXp);
    ui->monsterDetailsStatXpBar->repaint();
    ui->monsterDetailsStatAttackSpe->setText(tr("Special attack: %1").arg(stat.special_attack));
    ui->monsterDetailsStatDefenseSpe->setText(tr("Special defense: %1").arg(stat.special_defense));
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        ui->monsterDetailsStatSp->setVisible(true);
        ui->monsterDetailsStatSp->setText(tr("Skill point: %1").arg(monster.sp));
    }
    else
        ui->monsterDetailsStatSp->setVisible(false);
    //skill
    ui->monsterDetailsSkills->clear();
    {
        unsigned int indexSkill=0;
        while(indexSkill<monster.skills.size())
        {
            const PlayerMonster::PlayerSkill &monsterSkill=monster.skills.at(indexSkill);
            QListWidgetItem *item;
            if(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().find(monsterSkill.skill)==
                    QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().cend())
                item=new QListWidgetItem(tr("Unknown skill"));
            else
            {
                if(monsterSkill.level>1)
                    item=new QListWidgetItem(tr("%1 at level %2").arg(
                                                 QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                                                                        .at(monsterSkill.skill).name)).arg(monsterSkill.level));
                else
                    item=new QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(monsterSkill.skill).name));
                const Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(monsterSkill.skill);
                item->setText(item->text()+QStringLiteral(" (%1/%2)")
                        .arg(monsterSkill.endurance)
                        .arg(skill.level.at(monsterSkill.level-1).endurance)
                        );
                if(skill.type!=255)
                    if(QtDatapackClientLoader::datapackLoader->get_typeExtra().find(skill.type)!=QtDatapackClientLoader::datapackLoader->get_typeExtra().cend())
                        if(!QtDatapackClientLoader::datapackLoader->get_typeExtra().at(skill.type).name.empty())
                            item->setText(item->text()+", "+tr("Type: %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_typeExtra()
                                                                                                      .at(skill.type).name)));
                item->setText(item->text()+"\n"+QString::fromStdString(
                                  QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(monsterSkill.skill).description));
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(monsterSkill.skill).description));
            }
            ui->monsterDetailsSkills->addItem(item);
            indexSkill++;
        }
    }
    //do the buff
    {
        ui->monsterDetailsBuffs->clear();
        unsigned int index=0;
        while(index<monster.buffs.size())
        {
            const PlayerBuff &buffEffect=monster.buffs.at(index);
            QListWidgetItem *item=new QListWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().find(buffEffect.buff)==QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().cend())
            {
                item->setToolTip(tr("Unknown buff"));
                item->setIcon(QIcon(":/images/interface/buff.png"));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterBuffExtra(buffEffect.buff).icon);
                if(buffEffect.level<=1)
                    item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(buffEffect.buff).name));
                else
                    item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(
                          QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(buffEffect.buff).name)).arg(buffEffect.level));
                item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra()
                                                                             .at(buffEffect.buff).description));
            }
            ui->monsterDetailsBuffs->addItem(item);
            index++;
        }
    }

    ui->stackedWidget->setCurrentWidget(ui->page_monsterdetails);
}

//need correct match, else tp to die can failed and do mistake
bool BaseWindow::check_monsters()
{
    unsigned int index=0;
    unsigned int sub_size;
    unsigned int sub_index;
    while(index<client->player_informations.playerMonster.size())
    {
        const PlayerMonster &monster=client->player_informations.playerMonster.at(index);
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)==CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
        {
            error(QStringLiteral("the monster %1 is not into monster list").arg(monster.monster).toStdString());
            return false;
        }
        const Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster);
        if(monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            error(QStringLiteral("the level %1 is greater than max level").arg(monster.level).toStdString());
            return false;
        }
        Monster::Stat stat=CommonFightEngine::getStat(monsterDef,monster.level);
        if(monster.hp>stat.hp)
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(stat.hp).arg(monster.level).toStdString());
            return false;
        }
        if(monster.remaining_xp>monsterDef.level_to_xp.at(monster.level-1))
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(monster.remaining_xp).arg(monster.level).toStdString());
            return false;
        }
        sub_size=static_cast<uint32_t>(monster.buffs.size());
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().find(monster.buffs.at(sub_index).buff)==
                    CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().cend())
            {
                error(QStringLiteral("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff).toStdString());
                return false;
            }
            if(monster.buffs.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().at(monster.buffs.at(sub_index).buff).level.size())
            {
                error(QStringLiteral("the buff have not the level %1 is not into the buff list").arg(monster.buffs.at(sub_index).level).toStdString());
                return false;
            }
            sub_index++;
        }
        sub_size=static_cast<uint32_t>(monster.skills.size());
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().find(monster.skills.at(sub_index).skill)==
                    CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().cend())
            {
                error(QStringLiteral("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill).toStdString());
                return false;
            }
            if(monster.skills.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(monster.skills.at(sub_index).skill).level.size())
            {
                error(QStringLiteral("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level).toStdString());
                return false;
            }
            sub_index++;
        }
        index++;
    }
    //client->setPlayerMonster(client->player_informations.playerMonster);
    return true;
}

void BaseWindow::load_monsters()
{
    const std::vector<PlayerMonster> &playerMonsters=client->getPlayerMonster();
    ui->monsterList->clear();
    monsterspos_items_graphical.clear();
    if(playerMonsters.empty())
        return;
    const uint8_t &currentMonsterPosition=client->getCurrentSelectedMonsterNumber();
    unsigned int index=0;
    while(index<playerMonsters.size())
    {
        const PlayerMonster &monster=playerMonsters.at(index);
        if(inSelection)
        {
            if(waitedObjectType==ObjectType_MonsterToFight && index==currentMonsterPosition)
            {
                index++;
                continue;
            }
            switch(waitedObjectType)
            {
                case ObjectType_ItemEvolutionOnMonster:
                    if(monster.hp==0 || monster.egg_step>0)
                    {
                        index++;
                        continue;
                    }
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.find(objectInUsing.back())==
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.cend())
                    {
                        index++;
                        continue;
                    }
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(objectInUsing.back()).find(monster.monster)==
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(objectInUsing.back()).cend())
                    {
                        index++;
                        continue;
                    }
                break;
                case ObjectType_ItemLearnOnMonster:
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.find(objectInUsing.back())==
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.cend())
                    {
                        index++;
                        continue;
                    }
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.at(objectInUsing.back()).find(monster.monster)==
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.at(objectInUsing.back()).cend())
                    {
                        index++;
                        continue;
                    }
                break;
                case ObjectType_MonsterToFight:
                case ObjectType_MonsterToFightKO:
                case ObjectType_ItemOnMonster:
                    if(monster.hp==0 || monster.egg_step>0)
                    {
                        index++;
                        continue;
                    }
                break;
                default:
                break;
            }
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=
                CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
        {
            Monster::Stat stat=CatchChallenger::CommonFightEngine::getStat(
                        CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster),monster.level);

            QListWidgetItem *item=new QListWidgetItem();
            item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).description));
            if(!QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).thumb.isNull())
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).thumb);
            else
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front);

            if(waitedObjectType==ObjectType_MonsterToLearn && inSelection)
            {
                QHash<uint32_t,uint8_t> skillToDisplay;
                unsigned int sub_index=0;
                while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.size())
                {
                    Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.at(sub_index);
                    if(learn.learnAtLevel<=monster.level)
                    {
                        unsigned int sub_index2=0;
                        while(sub_index2<monster.skills.size())
                        {
                            const PlayerMonster::PlayerSkill &skill=monster.skills.at(sub_index2);
                            if(skill.skill==learn.learnSkill)
                                break;
                            sub_index2++;
                        }
                        if(
                                //if skill not found
                                (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                                ||
                                //if skill already found and need level up
                                (sub_index2<monster.skills.size() && (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
                        )
                        {
                            if(skillToDisplay.contains(learn.learnSkill))
                            {
                                if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
                                    skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                            }
                            else
                                skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                        }
                    }
                    sub_index++;
                }
                if(skillToDisplay.isEmpty())
                    item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("No skill to learn"))
                            );
                else
                    item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("%n skill(s) to learn","",skillToDisplay.size()))
                            );
            }
            else
            {
                item->setText(tr("%1, level: %2\nHP: %3/%4")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                        .arg(monster.level)
                        .arg(monster.hp)
                        .arg(stat.hp)
                        );
            }
            if(monster.hp==0)
                item->setBackground(QColor(255,220,220,255));
            else if(index==currentMonsterPosition)
            {
                if(!client->isInFight())
                    item->setBackground(QColor(200,255,255,255));
            }
            ui->monsterList->addItem(item);
            monsterspos_items_graphical[item]=static_cast<uint8_t>(index);
        }
        else
        {
            error(QStringLiteral("Unknown monster: %1").arg(monster.monster).toStdString());
            return;
        }
        index++;
    }
    if(inSelection)
    {
       ui->monsterListMoveUp->setVisible(false);
       ui->monsterListMoveDown->setVisible(false);
       ui->toolButton_monster_list_quit->setVisible(waitedObjectType!=ObjectType_MonsterToFightKO);
    }
    else
    {
        ui->monsterListMoveUp->setVisible(true);
        ui->monsterListMoveDown->setVisible(true);
        ui->toolButton_monster_list_quit->setVisible(true);
        on_monsterList_itemSelectionChanged();
    }
    ui->selectMonster->setVisible(true);
}

void BaseWindow::wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y)
{
    if(!fightCollision(map,x,y))
        return;
    prepareFight();
    battleType=BattleType_Wild;
    PublicPlayerMonster *otherMonster=client->getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at wildFightCollision()");
        return;
    }
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("A other %1 is in front of you!")
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(otherMonster->monster).name)));
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    resetPosition(true);
    moveFightMonsterBoth();
}

void BaseWindow::prepareFight()
{
    escape=false;
    escapeSuccess=false;
}

void BaseWindow::botFight(const uint16_t &fightId)
{
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(fightId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
    {
        emit error("fight id not found at collision");
        return;
    }
    botFightList.push_back(fightId);
    if(botFightList.size()>1 || !battleInformationsList.empty())
        return;
    botFightFull(fightId);
}

void BaseWindow::botFightFull(const uint16_t &fightId)
{
    this->fightId=fightId;
    botFightTimer.start();
}

void BaseWindow::botFightFullDiffered()
{
    prepareFight();
    ui->frameFightTop->setVisible(false);
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_botFightsExtra().at(fightId).start));
    ui->pushButtonFightEnterNext->setVisible(false);
    std::vector<PlayerMonster> botFightMonstersTransformed;
    const std::vector<BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().at(fightId).monsters;
    unsigned int index=0;
    while(index<monsters.size())
    {
        const BotFight::BotFightMonster &botFightMonster=monsters.at(index);
        botFightMonstersTransformed.push_back(FacilityLib::botFightMonsterToPlayerMonster(
                                           monsters.at(index),
                                           CommonFightEngine::getStat(
                                               CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(botFightMonster.id),
                                               monsters.at(index).level
                                               )
                                           ));
        index++;
    }
    client->setBotMonster(botFightMonstersTransformed,fightId);
    init_environement_display(mapController->getMapObject(),mapController->getX(),mapController->getY());
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    ui->pushButtonFightEnterNext->setVisible(false);
    battleType=BattleType_Bot;
    QPixmap botImage;
    if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        botImage=getFrontSkin(actualBot.properties.at("skin"));
    else
        botImage=QPixmap(QStringLiteral(":/images/player_default/front.png"));
    ui->labelFightMonsterTop->setPixmap(botImage.scaled(160,160));
    qDebug() << QStringLiteral("The bot %1 is a front of you").arg(fightId);
    battleStep=BattleStep_Presentation;
    moveType=MoveType_Enter;
    resetPosition(true);
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
}

bool BaseWindow::fightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y)
{
    if(!client->isInFight())
    {
        emit error("is in fight but without monster");
        return false;
    }
    init_environement_display(map,x,y);
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    init_other_monster_display();
    PublicPlayerMonster *otherMonster=client->getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at fightCollision()");
        return false;
    }
    qDebug() << QStringLiteral("You are in front of monster id: %1 level: %2 with hp: %3").arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonster->hp);
    return true;
}

void BaseWindow::init_environement_display(Map_client *map, const uint8_t &x, const uint8_t &y)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    Q_UNUSED(x);
    Q_UNUSED(y);
    //map not located
    if(map==NULL)
    {
        ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/background.png")).scaled(800,440));
        ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/foreground.png")).scaled(800,440));
        ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-front.png")).scaled(260,90));
        ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-background.png")).scaled(230,90));
        return;
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*map,x,y);
    unsigned int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(monstersCollisionValue.walkOn.at(index));
        if(monstersCollision.item==0 || playerInformations.items.find(monstersCollision.item)!=playerInformations.items.cend())
        {
            /*if(!monstersCollision.background.empty()) todo this part
            {
                const QString &baseSearch=QString::fromStdString(client->datapackPathBase())+DATAPACK_BASE_PATH_MAPBASE+
                        QString::fromStdString(monstersCollision.background);
                if(QFile(baseSearch+"/background.png").exists())
                    ui->labelFightBackground->setPixmap(QPixmap(baseSearch+QStringLiteral("/background.png")).scaled(800,440));
                else if(QFile(baseSearch+"/background.jpg").exists() &&
                        (supportedImageFormats.find("jpeg")!=supportedImageFormats.cend() ||
                                                                         supportedImageFormats.find("jpg")!=supportedImageFormats.cend()))
                    ui->labelFightBackground->setPixmap(QPixmap(baseSearch+QStringLiteral("/background.jpg")).scaled(800,440));
                else
                    ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/background.png")).scaled(800,440));

                if(QFile(baseSearch+"/foreground.png").exists())
                    ui->labelFightForeground->setPixmap(QPixmap(baseSearch+QStringLiteral("/foreground.png")).scaled(800,440));
                else if(QFile(baseSearch+"/foreground.gif").exists())
                    ui->labelFightForeground->setPixmap(QPixmap(baseSearch+QStringLiteral("/foreground.gif")).scaled(800,440));
                else
                    ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/foreground.png")).scaled(800,440));

                if(QFile(baseSearch+"/plateform-front.png").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-front.png")).scaled(260,90));
                else if(QFile(baseSearch+"/plateform-front.gif").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-front.gif")).scaled(260,90));
                else
                    ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-front.png")).scaled(260,90));

                if(QFile(baseSearch+"/plateform-background.png").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-background.png")).scaled(230,90));
                else if(QFile(baseSearch+"/plateform-background.gif").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-background.gif")).scaled(230,90));
                else
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-background.png")).scaled(230,90));
            }
            else*/
            {
                ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/background.png")).scaled(800,440));
                ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/foreground.png")).scaled(800,440));
                ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-front.png")).scaled(260,90));
                ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-background.png")).scaled(230,90));
            }
            return;
        }
        index++;
    }
    ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/background.png")).scaled(800,440));
    ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/foreground.png")).scaled(800,440));
    ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-front.png")).scaled(260,90));
    ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/images/interface/fight/plateform-background.png")).scaled(230,90));
}

void BaseWindow::init_other_monster_display()
{
    updateOtherMonsterInformation();
}

void BaseWindow::init_current_monster_display(PlayerMonster *fightMonster)
{
    if(fightMonster==NULL)
        fightMonster=client->getCurrentMonster();
    if(fightMonster!=NULL)
    {
        //current monster
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->stackedWidget->setCurrentWidget(ui->page_battle);
        ui->pushButtonFightEnterNext->setVisible(true);
        ui->frameFightBottom->setVisible(false);
        ui->labelFightBottomName->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(fightMonster->monster).name));
        ui->labelFightBottomLevel->setText(tr("Level %1").arg(fightMonster->level));
        Monster::Stat fightStat=CatchChallenger::CommonFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(fightMonster->monster),fightMonster->level);
        ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
        ui->progressBarFightBottomHP->setValue(fightMonster->hp);
        ui->progressBarFightBottomHP->repaint();
        ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(fightMonster->hp).arg(fightStat.hp));
        const Monster &monsterGenericInfo=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(fightMonster->monster);
        int level_to_xp=monsterGenericInfo.level_to_xp.at(fightMonster->level-1);
        ui->progressBarFightBottomExp->setMaximum(level_to_xp);
        ui->progressBarFightBottomExp->setValue(fightMonster->remaining_xp);
        ui->progressBarFightBottomExp->repaint();
        //do the buff
        {
            buffToGraphicalItemBottom.clear();
            ui->bottomBuff->clear();
            unsigned int index=0;
            while(index<fightMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=fightMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().find(buffEffect.buff)==
                        QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().cend())
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterBuffExtra(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(buffEffect.buff).name));
                    else
                        item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(
                                                                                             buffEffect.buff).name)).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(
                                                                                     buffEffect.buff).description));
                }
                buffToGraphicalItemBottom[buffEffect.buff]=item;
                item->setData(99,buffEffect.buff);//to prevent duplicate buff, because add can be to re-enable an already enable buff (for larger period then)
                ui->bottomBuff->addItem(item);
                index++;
            }
        }
    }
    else
        emit error("Try fight without monster");
    battleStep=BattleStep_PresentationMonster;
}

void BaseWindow::on_pushButtonFightEnterNext_clicked()
{
    ui->pushButtonFightEnterNext->setVisible(false);
    switch(battleStep)
    {
        case BattleStep_Presentation:
            if(client->isInFightWithWild())
            {
                moveType=MoveType_Leave;
                moveFightMonsterBottom();
            }
            else
            {
                moveType=MoveType_Leave;
                moveFightMonsterBoth();
            }
        break;
        case BattleStep_PresentationMonster:
        default:
            ui->frameFightTop->show();
            ui->frameFightBottom->show();
            moveType=MoveType_Enter;
            moveFightMonsterBottom();
            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
            PlayerMonster *monster=client->getCurrentMonster();
            if(monster!=NULL)
                ui->labelFightEnter->setText(tr("Protect me %1!").arg(QString::fromStdString(
                                                                          QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster->monster).name)));
            else
            {
                qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                ui->labelFightEnter->setText(tr("Protect me %1!").arg("(Unknow monster)"));
            }
        break;
    }
}

void BaseWindow::moveFightMonsterBottom()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().rx()<60)
            moveFightMonsterBottomTimer.start();
        else
        {
            updateCurrentMonsterInformation();
            doNextAction();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().rx()>(-ui->labelFightMonsterBottom->size().width()))
            moveFightMonsterBottomTimer.start();
        else
        {
            updateCurrentMonsterInformation();
            updateCurrentMonsterInformationXp();
            if(battleStep==BattleStep_Presentation)
            {
                resetPosition(true,false,true);
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                PlayerMonster *monster=client->getCurrentMonster();
                if(monster==NULL)
                {
                    newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                             "NULL pointer at updateCurrentMonsterInformation()");
                    return;
                }
                ui->labelFightEnter->setText(tr("Go %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster->monster).name)));
                moveFightMonsterBottom();
            }
        }
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setY(p.ry()+4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().ry()<440)
            moveFightMonsterBottomTimer.start();
        else
        {
            client->dropKOCurrentMonster();
            if(client->haveAnotherMonsterOnThePlayerToFight())
            {
                if(client->isInFight())
                {
                    #ifdef DEBUG_CLIENT_BATTLE
                    qDebug() << "Your current monster is KO, select another";
                    #endif
                    selectObject(ObjectType_MonsterToFightKO);
                }
                else
                {
                    #ifdef DEBUG_CLIENT_BATTLE
                    qDebug() << "You win";
                    #endif
                    displayText(tr("You win!").toStdString());
                    doNextActionStep=DoNextActionStep_Win;
                }
            }
            else
            {
                #ifdef DEBUG_CLIENT_BATTLE
                qDebug() << "You lose";
                #endif
                displayText(tr("You lose!").toStdString());
                doNextActionStep=DoNextActionStep_Loose;
            }
        }
    }
}

void BaseWindow::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(client->currentMonsterIsKO() && !client->haveAnotherMonsterOnThePlayerToFight())//then is dead, is teleported to the last rescue point
    {
        qDebug() << "tp on loose: " << fightTimerFinish;
        if(fightTimerFinish)
            loose();
        else
            fightTimerFinish=true;
    }
    else
        qDebug() << "normal tp";
}

void BaseWindow::updateCurrentMonsterInformationXp()
{
    PlayerMonster *monster=client->getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    ui->progressBarFightBottomExp->setValue(monster->remaining_xp);
    ui->progressBarFightBottomExp->repaint();
    ui->labelFightBottomLevel->setText(tr("Level %1").arg(monster->level));
    const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
    uint32_t maxXp=monsterInformations.level_to_xp.at(monster->level-1);
    ui->progressBarFightBottomExp->setMaximum(maxXp);
}

void BaseWindow::updateCurrentMonsterInformation()
{
    if(!client->getAbleToFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Try update the monster when have not any ready monster");
        return;
    }
    PlayerMonster *monster=client->getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    qDebug() << "Now visible monster have hp:" << monster->hp;
    #endif
    QPoint p;
    p.setX(60);
    p.setY(280);
    ui->labelFightMonsterBottom->move(p);
    ui->labelFightMonsterBottom->setPixmap(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster->monster).back.scaled(160,160));
    ui->frameFightBottom->setVisible(true);
    ui->frameFightBottom->show();
    currentMonsterLevel=monster->level;
    updateAttackList();
}

void BaseWindow::updateAttackList()
{
    if(!client->getAbleToFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Try update the monster when have not any ready monster");
        return;
    }
    PlayerMonster *monster=client->getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    unsigned int index=0;
    useTheRescueSkill=true;
    while(index<monster->skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::PlayerSkill &skill=monster->skills.at(index);
        if(skill.level>1)
            item->setText(QStringLiteral("%1, level %2 (remaining endurance: %3)")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(skill.skill).name))
                    .arg(skill.level)
                    .arg(skill.endurance)
                    );
        else
            item->setText(QStringLiteral("%1 (remaining endurance: %2)")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(skill.skill).name))
                    .arg(skill.endurance)
                    );
        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(skill.skill).description));
        item->setData(99,skill.endurance);
        item->setData(98,skill.skill);
        if(skill.endurance>0)
            useTheRescueSkill=false;
        fight_attacks_graphical[item]=skill.skill;
        ui->listWidgetFightAttack->addItem(item);
        index++;
    }
    if(useTheRescueSkill && CommonDatapack::commonDatapack.get_monsterSkills().find(0)!=CommonDatapack::commonDatapack.get_monsterSkills().cend())
        if(!CommonDatapack::commonDatapack.get_monsterSkills().at(0).level.empty())
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QStringLiteral("Rescue skill: %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(0).name)));
            item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(0).description));
            item->setData(99,0);
            fight_attacks_graphical[item]=0;
            ui->listWidgetFightAttack->addItem(item);
        }
    on_listWidgetFightAttack_itemSelectionChanged();
}

void BaseWindow::moveFightMonsterTop()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()>510)
            moveFightMonsterTopTimer.start();
        else
        {
            updateOtherMonsterInformation();
            doNextAction();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800)
            moveFightMonsterTopTimer.start();
        else
        {
            updateOtherMonsterInformation();
            doNextAction();
        }
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800)
            moveFightMonsterTopTimer.start();
        else
        {
            client->dropKOOtherMonster();
            if(client->isInFight())
            {
                if(client->isInBattle() && !client->haveBattleOtherMonster())
                {
                    ui->pushButtonFightEnterNext->setVisible(false);
                    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                    ui->labelFightEnter->setText(tr("In waiting of other monster selection"));
                    return;
                }
                ui->pushButtonFightEnterNext->setVisible(false);
                init_other_monster_display();
                ui->frameFightTop->setVisible(false);
                updateOtherMonsterInformation();
                ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                PublicPlayerMonster *otherMonster=client->getOtherMonster();
                if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(otherMonster->monster)!=
                        QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(
                                                     QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(otherMonster->monster).name)));
                else
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(tr("(Unknown monster)")));
                resetPosition(true,true,false);
                moveType=MoveType_Enter;
                moveFightMonsterTop();
            }
            else
            {
                qDebug() << "doNextAction(): you win";
                doNextActionStep=DoNextActionStep_Win;
                if(!escape)
                {
                    PlayerMonster *currentMonster=client->getCurrentMonster();
                    if(currentMonster!=NULL)
                    {
                        ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
                        ui->progressBarFightBottomExp->repaint();
                        ui->labelFightBottomLevel->setText(tr("Level %1").arg(currentMonster->level));
                    }
                    displayText(tr("You win!").toStdString());
                }
                else
                {
                    if(escapeSuccess)
                        displayText(tr("Your escape is successful").toStdString());
                    else
                        displayText(tr("Your escape have failed but you win").toStdString());
                }
                if(escape)
                {
                    fightTimerFinish=true;
                    doNextAction();
                }
            }
        }
    }
}

void BaseWindow::moveFightMonsterBoth()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterTop->move(p);
        p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()+3);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterTop->pos().rx()>510 || ui->labelFightMonsterBottom->pos().rx()<(60))
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                resetPosition();
                ui->pushButtonFightEnterNext->show();
            }
            else if(battleStep==BattleStep_PresentationMonster)
            {
                doNextActionStep=DoNextActionStep_Start;
                doNextAction();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()-3);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800 || ui->labelFightMonsterBottom->pos().rx()<(-ui->labelFightMonsterBottom->size().width()))
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                init_current_monster_display();
                ui->pushButtonFightEnterNext->setVisible(false);
                init_other_monster_display();
                ui->frameFightTop->setVisible(false);
                updateCurrentMonsterInformation();
                updateOtherMonsterInformation();
                resetPosition(true);
                QString text;
                PublicPlayerMonster *otherMonster=client->getOtherMonster();
                if(otherMonster!=NULL)
                    text+=tr("The other player call %1!").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                                                                                         otherMonster->monster).name));
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("The other player call %1!").arg("(Unknow monster)");
                }
                text+="<br />";
                PublicPlayerMonster *monster=client->getCurrentMonster();
                if(monster!=NULL)
                    text+=tr("You call %1!").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster->monster).name));
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("You call %1!").arg("(Unknow monster)");
                }
                ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                ui->labelFightEnter->setText(text);
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                moveFightMonsterBoth();
            }
            else if(battleStep==BattleStep_PresentationMonster)
            {
                doNextActionStep=DoNextActionStep_Start;
                doNextAction();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setY(p.ry()+4);
        ui->labelFightMonsterBottom->move(p);
        p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800 || ui->labelFightMonsterBottom->pos().ry()<440)
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
                init_current_monster_display();
                init_other_monster_display();
                updateCurrentMonsterInformation();
                updateOtherMonsterInformation();
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                moveFightMonsterBoth();
                ui->frameFightTop->show();
                ui->frameFightBottom->show();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
}

void BaseWindow::updateOtherMonsterInformation()
{
    if(!client->isInFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"updateOtherMonsterInformation() but have not other monter");
        return;
    }
    QPoint p;
    p.setX(510);
    p.setY(90);
    ui->labelFightMonsterTop->move(p);
    PublicPlayerMonster *otherMonster=client->getOtherMonster();
    if(otherMonster!=NULL)
    {
        ui->labelFightMonsterTop->setPixmap(QtDatapackClientLoader::datapackLoader->getMonsterExtra(otherMonster->monster).front.scaled(160,160));
        //other monster
        ui->frameFightTop->setVisible(true);
        ui->frameFightTop->show();
        ui->labelFightTopName->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(otherMonster->monster).name));
        ui->labelFightTopLevel->setText(tr("Level %1").arg(otherMonster->level));
        Monster::Stat otherStat=CatchChallenger::CommonFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(otherMonster->monster),otherMonster->level);
        ui->progressBarFightTopHP->setMaximum(otherStat.hp);
        ui->progressBarFightTopHP->setValue(otherMonster->hp);
        ui->progressBarFightTopHP->repaint();
        //do the buff
        {
            buffToGraphicalItemTop.clear();
            ui->topBuff->clear();
            unsigned int index=0;
            while(index<otherMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=otherMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().find(buffEffect.buff)==
                        QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().cend())
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterBuffExtra(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(buffEffect.buff).name));
                    else
                        item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(
                                                                                             buffEffect.buff).name)).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterBuffsExtra().at(buffEffect.buff)
                                                                                 .description));
                }
                buffToGraphicalItemTop[buffEffect.buff]=item;
                item->setData(99,buffEffect.buff);//to prevent duplicate buff, because add can be to re-enable an already enable buff (for larger period then)
                ui->topBuff->addItem(item);
                index++;
            }
        }
    }
    else
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Unable to load the other monster");
        return;
    }
}

void BaseWindow::on_toolButtonFightQuit_clicked()
{
    if(!client->canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=true;
    qDebug() << "BaseWindow::on_toolButtonFightQuit_clicked(): fight engine tryEscape()";
    if(static_cast<Api_protocol_Qt *>(client)->tryEscape())
        escapeSuccess=true;
    else
        escapeSuccess=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    doNextAction();
}

void BaseWindow::on_pushButtonFightAttack_clicked()
{
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageAttack);
}

void BaseWindow::on_pushButtonFightMonster_clicked()
{
    selectObject(ObjectType_MonsterToFight);
}

void BaseWindow::on_pushButtonFightAttackConfirmed_clicked()
{
    if(!client->canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    QList<QListWidgetItem *> itemsList=ui->listWidgetFightAttack->selectedItems();
    if(itemsList.size()!=1)
    {
        QMessageBox::warning(this,tr("Selection error"),tr("You need select an attack"));
        return;
    }
    const uint8_t skillEndurance=static_cast<uint8_t>(itemsList.first()->data(99).toUInt());
    const uint16_t skillUsed=fight_attacks_graphical.at(itemsList.first());
    if(skillEndurance<=0 && !useTheRescueSkill)
    {
        QMessageBox::warning(this,tr("No endurance"),tr("You have no more endurance to use this skill"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    PlayerMonster *monster=client->getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    if(useTheRescueSkill)
        client->useSkill(0);
    else
        client->useSkill(skillUsed);
    updateAttackList();
    displayAttackProgression=0;
    attack_quantity_changed=0;
    if(battleType!=BattleType_OtherPlayer)
        doNextAction();
    else
    {
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("In waiting of the other player action"));
        ui->pushButtonFightEnterNext->hide();
    }
}

void BaseWindow::on_pushButtonFightReturn_clicked()
{
    ui->listWidgetFightAttack->reset();
    on_listWidgetFightAttack_itemSelectionChanged();
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
}

void BaseWindow::on_listWidgetFightAttack_itemSelectionChanged()
{
    QList<QListWidgetItem *> itemsList=ui->listWidgetFightAttack->selectedItems();
    if(itemsList.size()!=1)
    {
        ui->labelFightAttackDetails->setText(tr("Select an attack"));
        return;
    }
    uint16_t skillId=fight_attacks_graphical.at(itemsList.first());
    ui->labelFightAttackDetails->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(skillId).description));
}

void BaseWindow::checkEvolution()
{
    PlayerMonster *monster=client->evolutionByLevelUp();
    if(monster!=NULL)
    {
        const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
        const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster->monster);
        unsigned int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Level && evolution.data.level==monster->level)
            {
                monsterEvolutionPostion=client->getPlayerMonsterPosition(monster);
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.get_monsters().at(evolution.evolveTo);
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                            evolution.evolveTo);
                //create animation widget
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                return;
            }
            index++;
        }
    }
    while(!tradeEvolutionMonsters.empty())
    {
        const uint8_t monsterPosition=tradeEvolutionMonsters.at(0);
        PlayerMonster * const playerMonster=client->monsterByPosition(monsterPosition);
        if(playerMonster==NULL)
        {
            tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
            continue;
        }
        const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(playerMonster->monster);
        const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(playerMonster->monster);
        unsigned int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Trade)
            {
                monsterEvolutionPostion=monsterPosition;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.get_monsters().at(evolution.evolveTo);
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra()
                        .at(evolution.evolveTo);
                //create animation widget
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
                return;
            }
            index++;
        }
        tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
    }
}

void BaseWindow::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    checkEvolution();
}

void BaseWindow::loose()
{
    qDebug() << "loose()";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=client->getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString()
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=client->getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString()
                       );
            return;
        }
    #endif
    if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().empty())
        return;
    client->healAllMonsters();
    client->fightFinished();
    mapController->unblock();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(botFightList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.erase(botFightList.cbegin());
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.erase(battleInformationsList.cbegin());
        break;
        default:
        break;
    }
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
    checkEvolution();
}

void BaseWindow::win()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=client->getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString());
            return;
        }
    PublicPlayerMonster *otherMonster=client->getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString());
            return;
        }
    #endif
    client->fightFinished();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win end && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString());
            return;
        }
    #endif
    mapController->unblock();
    if(zonecatch)
        ui->stackedWidget->setCurrentWidget(ui->page_zonecatch);
    else
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(!zonecatch)
            {
                if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(fightId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
                {
                    emit error("fight id not found at collision");
                    return;
                }
                addCash(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().at(fightId).cash);
                mapController->addBeatenBotFight(fightId);
            }
            if(botFightList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.erase(botFightList.cbegin());
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.erase(battleInformationsList.cbegin());
        break;
        default:
        break;
    }
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        const uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
    checkEvolution();
}

void BaseWindow::pageChanged()
{
    if(ui->stackedWidget->currentWidget()==ui->page_map)
        mapController->setFocus();
    if(ui->stackedWidget->currentWidget()==ui->page_plants)
        load_plant_inventory();
}

void BaseWindow::displayExperienceGain()
{
    PlayerMonster * currentMonster=client->getCurrentMonster();
    if(currentMonster==NULL)
    {
        mLastGivenXP=0;
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    if(ui->progressBarFightBottomExp->value()==ui->progressBarFightBottomExp->maximum())
    {
        ui->progressBarFightBottomExp->setValue(0);
        ui->progressBarFightBottomExp->repaint();
    }
    //animation finished
    if(mLastGivenXP<=0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mLastGivenXP>4294000000)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part0: mLastGivenXP is negative").toStdString());
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("par0: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                     .toStdString());
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        #endif
        if(currentMonsterLevel==currentMonster->level && (uint32_t)ui->progressBarFightBottomExp->value()>currentMonster->remaining_xp)
        {
            message(QStringLiteral("part0: displayed xp greater than the real xp: %1>%2, auto fix")
                    .arg(ui->progressBarFightBottomExp->value()).arg(currentMonster->remaining_xp).toStdString());
            ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        doNextAction();
        return;
    }

    //if start, display text
    if(displayAttackProgression==0)
    {
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("You %1 gain %2 of experience").arg(
                                         QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(currentMonster->monster).name))
                                     .arg(mLastGivenXP));
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part1: mLastGivenXP is negative").toStdString());
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part1: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level).toStdString());
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    uint32_t xp_to_change;
    if(currentMonsterLevel<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        xp_to_change=ui->progressBarFightBottomExp->maximum()/200;//0.5%
        if(xp_to_change==0)
            xp_to_change=1;
        if(xp_to_change>mLastGivenXP)
            xp_to_change=mLastGivenXP;
    }
    else
    {
        xp_to_change=0;
        mLastGivenXP=0;
        doNextAction();
        return;
    }

    uint32_t maxXp=ui->progressBarFightBottomExp->maximum();
    if(((uint32_t)ui->progressBarFightBottomExp->value()+xp_to_change)>=(uint32_t)maxXp)
    {
        xp_to_change=maxXp-ui->progressBarFightBottomExp->value();
        if(xp_to_change>mLastGivenXP)
            xp_to_change=mLastGivenXP;
        if(mLastGivenXP>xp_to_change)
            mLastGivenXP-=xp_to_change;
        else
            mLastGivenXP=0;

        const Monster::Stat &oldStat=client->getStat(CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonsterLevel);
        const Monster::Stat &newStat=client->getStat(CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonsterLevel+1);
        if(oldStat.hp<newStat.hp)
        {
            qDebug() << QStringLiteral("Now the old hp: %1/%2 increased of %3 for the old level %4").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()).arg(newStat.hp-oldStat.hp).arg(currentMonsterLevel);
            ui->progressBarFightBottomHP->setMaximum(ui->progressBarFightBottomHP->maximum()+(newStat.hp-oldStat.hp));
            ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+(newStat.hp-oldStat.hp));
            ui->progressBarFightBottomHP->repaint();
            ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
        }
        else
            qDebug() << QStringLiteral("The hp at level change is untouched: %1/%2 for the old level %3").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()).arg(currentMonsterLevel);
        ui->progressBarFightBottomExp->setMaximum(CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster).level_to_xp.at(currentMonsterLevel-1));
        ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->maximum());
        ui->progressBarFightBottomExp->repaint();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mLastGivenXP>4294000000)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part2: mLastGivenXP is negative").toStdString());
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part2: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                     .toStdString());
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        #endif
        if(currentMonsterLevel>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        currentMonsterLevel++;
        ui->labelFightBottomLevel->setText(tr("Level %1").arg(currentMonsterLevel));
        displayExpTimer.start();
        displayAttackProgression++;
        return;
    }
    else
    {
        if(currentMonsterLevel<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->value()+xp_to_change);
            ui->progressBarFightBottomExp->repaint();
        }
        else
        {
            mLastGivenXP=0;
            doNextAction();
            return;
        }
    }
    if(mLastGivenXP>xp_to_change)
        mLastGivenXP-=xp_to_change;
    else
        mLastGivenXP=0;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part3: mLastGivenXP is negative").toStdString());
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part3: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                 .toStdString());
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    displayExpTimer.start();
    displayAttackProgression++;
}

void BaseWindow::displayTrap()
{
    if(client==nullptr)
        return;
    PublicPlayerMonster * otherMonster=client->getOtherMonster();
    PublicPlayerMonster * currentMonster=client->getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to use trap");
        doNextAction();
        return;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    //if start, display text
    if(displayTrapProgression==0)
    {
        if(movie!=NULL)
        {
            movie->stop();
            delete movie;
        }
        QString skillAnimation=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_ITEM;
        QString fileAnimation=skillAnimation+QStringLiteral("%1.mng").arg(trapItemId);
        if(QFile(fileAnimation).exists())
        {
            movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
            movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
            if(movie->isValid())
            {
                ui->labelFightTrap->setMovie(movie);
                movie->start();
            }
            else
                qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
        }
        else
        {
            fileAnimation=skillAnimation+QStringLiteral("%1.gif").arg(trapItemId);
            if(QFile(fileAnimation).exists())
            {
                movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                if(movie->isValid())
                {
                    ui->labelFightTrap->setMovie(movie);
                    movie->start();
                }
                else
                    qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
            }
        }
        ui->labelFightEnter->setText(QStringLiteral("Try catch the wild %1")
                      .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(otherMonster->monster).name)));
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        displayTrapProgression=1;
        ui->labelFightTrap->show();
    }
    uint32_t animationTime;
    if(displayTrapProgression==1)
        animationTime=1500;
    else
        animationTime=2000;
    if(displayTrapProgression==1 && (uint32_t)updateTrapTime.elapsed()<animationTime)
        ui->labelFightTrap->move(
                    ui->labelFightMonsterBottom->pos().x()+(ui->labelFightMonsterTop->pos().x()-ui->labelFightMonsterBottom->pos().x())*updateTrapTime.elapsed()/animationTime,
                    ui->labelFightMonsterBottom->pos().y()-(ui->labelFightMonsterBottom->pos().y()-ui->labelFightMonsterTop->pos().y())*updateTrapTime.elapsed()/animationTime
                    );
    else
        ui->labelFightTrap->move(ui->labelFightMonsterTop->pos().x(),ui->labelFightMonsterTop->pos().y());
    if(!client->playerMonster_catchInProgress.empty())
        return;
    if((uint32_t)updateTrapTime.elapsed()>animationTime)
    {
        updateTrapTime.restart();
        if(displayTrapProgression==1)
        {
            displayTrapProgression=2;
            if(movie!=NULL)
            {
                movie->stop();
                delete movie;
            }
            QString skillAnimation=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_ITEM;
            QString fileAnimation;
            if(trapSuccess)
                fileAnimation=skillAnimation+QStringLiteral("%1_success.mng").arg(trapItemId);
            else
                fileAnimation=skillAnimation+QStringLiteral("%1_failed.mng").arg(trapItemId);
            if(QFile(fileAnimation).exists())
            {
                movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                if(movie->isValid())
                {
                    ui->labelFightTrap->setMovie(movie);
                    movie->start();
                }
                else
                    qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
            }
            else
            {
                if(trapSuccess)
                    fileAnimation=skillAnimation+QStringLiteral("%1_success.gif").arg(trapItemId);
                else
                    fileAnimation=skillAnimation+QStringLiteral("%1_failed.gif").arg(trapItemId);
                if(QFile(fileAnimation).exists())
                {
                    movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                    movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                    if(movie->isValid())
                    {
                        ui->labelFightTrap->setMovie(movie);
                        movie->start();
                    }
                    else
                        qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
                }
            }
        }
        else
        {
            if(trapSuccess)
            {
                client->catchIsDone();//why? do at the screen change to return on the map, not here!
                displayText(tr("You have catched the wild %1").arg(QString::fromStdString(
                                                                       QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                                                                           otherMonster->monster).name)).toStdString());
            }
            else
                displayText(tr("You have failed the catch of the wild %1").arg(QString::fromStdString(
                                                                                   QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                                                                                       otherMonster->monster).name)).toStdString());
            ui->labelFightTrap->hide();
            return;
        }
    }
    displayTrapTimer.start();
}

void BaseWindow::displayText(const std::string &text)
{
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(QString::fromStdString(text));
    doNextActionTimer.start();
}

void BaseWindow::resetPosition(bool outOfScreen,bool topMonster,bool bottomMonster)
{
    QPoint p;
    if(outOfScreen)
    {
        if(bottomMonster)
        {
            p.setX(-160);
            p.setY(280);
            ui->labelFightMonsterBottom->move(p);
        }
        if(topMonster)
        {
            p.setX(800);
            p.setY(90);
            ui->labelFightMonsterTop->move(p);
        }
    }
    else
    {
        if(bottomMonster)
        {
            p.setX(60);
            p.setY(280);
            ui->labelFightMonsterBottom->move(p);
        }
        if(topMonster)
        {
            p.setX(510);
            p.setY(90);
            ui->labelFightMonsterTop->move(p);
        }
    }
}

void BaseWindow::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    tradeOtherMonsters.push_back(monster);
    QString genderString;
    switch(monster.gender)
    {
        case 0x01:
        genderString=tr("Male");
        break;
        case 0x02:
        genderString=tr("Female");
        break;
        default:
        genderString=tr("Unknown");
        break;
    }
    QListWidgetItem *item=new QListWidgetItem();
    if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(monster.monster)!=
            QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front);
        item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name));
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/front.png"));
        item->setText(tr("Unknown"));
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    ui->tradeOtherMonsters->addItem(item);
}

void BaseWindow::on_learnQuit_clicked()
{
    selectObject(ObjectType_MonsterToLearn);
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_learnValidate_clicked()
{
    QList<QListWidgetItem *> itemsList=ui->learnAttackList->selectedItems();
    if(itemsList.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select an attack to learn"));
        return;
    }
    on_learnAttackList_itemActivated(itemsList.first());
}

void BaseWindow::on_learnAttackList_itemActivated(QListWidgetItem *item)
{
    if(attack_to_learn_graphical.find(item)==attack_to_learn_graphical.cend())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Selected item wrong");
        return;
    }
    if(!learnSkillByPosition(monsterPositionToLearn,attack_to_learn_graphical.at(item)))
    {
        ui->learnDescription->setText(tr("You can't learn this attack"));
        return;
    }
    if(monsterspos_items_graphical.find(item)==monsterspos_items_graphical.cend())
        return;
    showLearnSkillByPosition(monsterPositionToLearn);
}

bool BaseWindow::learnSkillByPosition(const uint8_t &monsterPosition,const uint16_t &skill)
{
    if(!showLearnSkillByPosition(monsterPosition))
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Unable to load the right monster");
        return false;
    }
    if(client->learnSkill(client->monsterByPosition(monsterPosition),skill))
    {
        client->learnSkillByPosition(monsterPosition,skill);
        showLearnSkillByPosition(monsterPosition);
        return true;
    }
    return false;
}

//learn
bool BaseWindow::showLearnSkillByPosition(const uint8_t &monsterPosition)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->learnAttackList->clear();
    attack_to_learn_graphical.clear();
    std::vector<PlayerMonster> playerMonster=client->getPlayerMonster();
    //get the right monster
    QHash<uint16_t,uint8_t> skillToDisplay;
    unsigned int index=monsterPosition;
    PlayerMonster monster=playerMonster.at(index);
    ui->learnMonster->setPixmap(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front.scaled(160,160));
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        ui->learnSP->setVisible(true);
        ui->learnSP->setText(tr("SP: %1").arg(monster.sp));
    }
    else
        ui->learnSP->setVisible(false);
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    ui->learnInfo->setText(tr("<b>%1</b><br />Level %2").arg(
                               QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                           .arg(monster.level));
    #endif
    unsigned int sub_index=0;
    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.size())
    {
        Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monster.level)
        {
            unsigned int sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                const PlayerMonster::PlayerSkill &skill=monster.skills.at(sub_index2);
                if(skill.skill==learn.learnSkill)
                    break;
                sub_index2++;
            }
            if(
                    //if skill not found
                    (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                    ||
                    //if skill already found and need level up
                    (sub_index2<monster.skills.size() && (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
            )
            {
                if(skillToDisplay.contains(learn.learnSkill))
                {
                    if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
                        skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                }
                else
                    skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
            }
        }
        sub_index++;
    }
    QHashIterator<uint16_t,uint8_t> i(skillToDisplay);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        const uint32_t &level=i.value();
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(level<=1)
                item->setText(tr("%1\nSP cost: %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(i.key()).name))
                            .arg(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
            else
                item->setText(tr("%1 level %2\nSP cost: %3")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(i.key()).name))
                            .arg(level)
                            .arg(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
        }
        else
        {
            if(level<=1)
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(i.key()).name));
            else
                item->setText(tr("%1 level %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().at(i.key()).name)
                            .arg(level)
                        ));
        }
        if(CommonSettingsServer::commonSettingsServer.useSP && CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(i.key()).level.at(i.value()-1).sp_to_learn>monster.sp)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
            item->setToolTip(tr("You need more sp"));
        }
        attack_to_learn_graphical[item]=i.key();
        ui->learnAttackList->addItem(item);
    }
    if(attack_to_learn_graphical.empty())
        ui->learnDescription->setText(tr("No more attack to learn"));
    else
        ui->learnDescription->setText(tr("Select attack to learn"));
    return true;
}

void BaseWindow::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=client->getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster was hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=client->getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster was hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    client->addAndApplyAttackReturnList(attackReturn);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=client->getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster have hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=client->getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster have hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    doNextAction();
}

void BaseWindow::on_listWidgetFightAttack_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    on_pushButtonFightAttackConfirmed_clicked();
}

void BaseWindow::useTrap(const uint16_t &itemId)
{
    updateTrapTime.restart();
    displayTrapProgression=0;
    trapItemId=itemId;
    if(!client->tryCatchClient(itemId))
    {
        std::cerr << "!client->tryCatchClient(itemId)" << std::endl;
        abort();
    }
    displayTrap();
    //need wait the server reply, monsterCatch(const bool &success)
}

void BaseWindow::monsterCatch(const bool &success)
{
    if(client==nullptr)
    {
        emit error("client==nullptr BaseWindow::monsterCatch()");
        return;
    }
    if(client->playerMonster_catchInProgress.empty())
    {
        emit error("Internal bug: cupture monster list is emtpy");
        return;
    }
    if(!success)
    {
        trapSuccess=false;
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        emit message(QStringLiteral("catch is failed"));
        #endif
        client->generateOtherAttack();
    }
    else
    {
        trapSuccess=true;
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        emit message(QStringLiteral("catch is success"));
        #endif
        //client->playerMonster_catchInProgress.first().id=newMonsterId;
        if(client->getPlayerMonster().size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
        {
            Player_private_and_public_informations &playerInformations=client->get_player_informations();
            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
            {
                QMessageBox::warning(this,tr("Error"),tr("You have already the maximum number of monster into you warehouse"));
                return;
            }
            if(client->playerMonster_catchInProgress.empty())
                abort();
            const PlayerMonster &p=client->playerMonster_catchInProgress.front();
            playerInformations.warehouse_playerMonster.push_back(p);
        }
        else
        {
            if(client->playerMonster_catchInProgress.empty())
                abort();
            const PlayerMonster &p=client->playerMonster_catchInProgress.front();
            client->addPlayerMonster(p);
            load_monsters();
        }
    }
    client->playerMonster_catchInProgress.erase(client->playerMonster_catchInProgress.cbegin());
    displayTrap();
}

void BaseWindow::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,
                                       const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    BattleInformations battleInformations;
    battleInformations.pseudo=pseudo;
    battleInformations.skinId=skinId;
    battleInformations.stat=stat;
    battleInformations.monsterPlace=monsterPlace;
    battleInformations.publicPlayerMonster=publicPlayerMonster;
    battleInformationsList.push_back(battleInformations);
    if(battleInformationsList.size()>1 || !botFightList.empty())
        return;
    battleAcceptedByOtherFull(battleInformations);
}

void BaseWindow::battleAcceptedByOtherFull(const BattleInformations &battleInformations)
{
    if(client->isInFight())
    {
        qDebug() << "already in fight";
        client->battleRefused();
        return;
    }
    prepareFight();
    battleType=BattleType_OtherPlayer;
    ui->stackedWidget->setCurrentWidget(ui->page_battle);

    //skinFolderList=CatchChallenger::FacilityLib::skinIdList(client->get_datapack_base_name()+QStringLiteral(DATAPACK_BASE_PATH_SKIN));
    QPixmap otherFrontImage=getFrontSkin(battleInformations.skinId);

    //reset the other player info
    ui->labelFightMonsterTop->setPixmap(otherFrontImage.scaled(160,160));
    //ui->battleOtherPseudo->setText(lastBattleQuery.first().first);
    ui->frameFightTop->hide();
    ui->frameFightBottom->hide();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("%1 wish fight with you").arg(QString::fromStdString(battleInformations.pseudo)));
    ui->pushButtonFightEnterNext->hide();

    resetPosition(true);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
    client->setBattleMonster(battleInformations.stat,battleInformations.monsterPlace,battleInformations.publicPlayerMonster);

    init_environement_display(mapController->getMapObject(),mapController->getX(),mapController->getY());
}

void BaseWindow::battleCanceledByOther()
{
    client->fightFinished();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled the battle").toStdString());
    load_monsters();
    if(battleInformationsList.empty())
    {
        emit error("battle info not found at collision");
        return;
    }
    battleInformationsList.erase(battleInformationsList.cbegin());
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        const uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
}
