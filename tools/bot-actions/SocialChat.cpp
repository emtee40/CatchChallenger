#include "SocialChat.h"
#include "ui_SocialChat.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../client/libqtcatchchallenger/Api_client_real.hpp"
#include "../../client/qtcpu800x600/base/ChatParsing.hpp"
#include "DatabaseBot.h"

#include <QListWidget>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QScrollBar>
#include <iostream>

SocialChat * SocialChat::socialChat=NULL;

SocialChat::SocialChat() :
    ui(new Ui::SocialChat)
{
    ui->setupUi(this);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    completer=NULL;
    lastText.push_back(std::unordered_set<std::string>());
    if(!connect(ui->globalChatText,&QLineEdit::textChanged,this,&SocialChat::globalChatText_updateCompleter,Qt::QueuedConnection))
        abort();
    if(!connect(ui->globalChatText,&QLineEdit::cursorPositionChanged,this,&SocialChat::globalChatText_updateCompleter,Qt::QueuedConnection))
        abort();

    {
        QSqlQuery query;
        if(!query.prepare("SELECT * FROM globalchat"))
            abort();
        if(!query.exec())
        {
            qDebug() << "note load error: " << query.lastError();
            abort();
        }
        else while(query.next())
        {
            ChatEntry newEntry;
            newEntry.player_type=(CatchChallenger::Player_type)query.value("player_type").toUInt();
            const QString &pseudo=query.value("pseudo").toString();
            newEntry.player_pseudo=pseudo.toStdString();
            if(!pseudo.isEmpty())
                knownGlobalChatPlayers << pseudo;
            newEntry.chat_type=(CatchChallenger::Chat_type)query.value("chat_type").toUInt();
            newEntry.text=query.value("text").toString().toStdString();
            chat_list << newEntry;
        }

        while(chat_list.size()>64)
            chat_list.removeFirst();
        update_chat();
    }
}

void SocialChat::showEvent(QShowEvent * event)
{
    bool first=true;
    for (const auto &n:ActionsBotInterface::clientList) {
        CatchChallenger::Api_protocol_Qt * const api=n.first;
        ActionsBotInterface::Player &client=const_cast<ActionsBotInterface::Player &>(n.second);
        if(client.api->getCaracterSelected())
        {
            if(first==true)
            {
                if(!connect(client.api,&CatchChallenger::Api_protocol_Qt::Qtnew_system_text,this,&SocialChat::new_system_text))
                    abort();
                if(!connect(client.api,&CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,this,&SocialChat::new_chat_text))
                    abort();
                first=false;
            }
            const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client.api->get_player_informations();
            const QString &qtpseudo=QString::fromStdString(player_private_and_public_informations.public_informations.pseudo);
            ui->bots->addItem(qtpseudo);
            pseudoToBot[qtpseudo]=api;
            client.regexMatchPseudo=QRegularExpression("^(.*[ @])?"+QRegularExpression::escape(qtpseudo)+"([ @].*)?$");
            if(!client.regexMatchPseudo.isValid())
            {
                qDebug() << client.regexMatchPseudo.errorString();
                abort();
            }

            if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtinsert_player,            this,&SocialChat::insert_player))
                abort();
            if(!connect(api,&CatchChallenger::Api_protocol_Qt::QtdropAllPlayerOnTheMap,    this,&SocialChat::dropAllPlayerOnTheMap))
                abort();
            if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtremove_player,            this,&SocialChat::remove_player))
                abort();
        }
    }

    if(!event->spontaneous())
    {
        if(ui->bots->count()==1)
        {
            QListWidgetItem * item=ui->bots->itemAt(0,0);
            if(!item->isSelected())
            {
                item->setSelected(true);
                on_bots_itemSelectionChanged();
            }
            ui->groupBoxBot->setVisible(false);
        }
        std::cout << "QShowEvent: QEvent::Type: " << (int)event->type() << std::endl;
    }
    QWidget::showEvent(event);
}

void SocialChat::focusInEvent(QFocusEvent * event)
{
    (void)event;
    if(windowTitle().endsWith("*"))
    {
        QString title=windowTitle();
        title.remove("*");
        setWindowTitle(title);
    }
}

SocialChat::~SocialChat()
{
    delete ui;
    if(completer!=NULL)
    {
        delete completer;
        completer=NULL;
    }
}

void SocialChat::on_bots_itemSelectionChanged()
{
    ui->groupBoxVisiblePlayers->setEnabled(true);
    ui->groupBoxChat->setEnabled(true);
    ui->groupBoxInformations->setEnabled(true);

    loadPlayerInformation();
}

void SocialChat::loadPlayerInformation()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * api=pseudoToBot.value(pseudo);
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;

    selectedItems.at(0)->setBackground(Qt::NoBrush);
    if(windowTitle().endsWith("*"))
    {
        QString title=windowTitle();
        title.remove("*");
        setWindowTitle(title);
    }

    const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
    ui->labelPseudo->setText(pseudo);
    ui->chatSpecText->setPlaceholderText(tr("Your text as %1").arg(pseudo));
    ui->globalChatText->setPlaceholderText(tr("Your text as %1").arg(pseudo));

    switch(playerInformations.public_informations.type)
    {
        default:
    case CatchChallenger::Player_type_normal:
            ui->player_informations_type->setText(tr("Normal player"));
        break;
        case CatchChallenger::Player_type_premium:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/premium.png"));
        break;
        case CatchChallenger::Player_type_dev:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/developer.png"));
        break;
        case CatchChallenger::Player_type_gm:
            ui->player_informations_type->setPixmap(QPixmap(":/images/chat/admin.png"));
        break;
    }

    //front image
    if(playerInformations.public_informations.skinId<QtDatapackClientLoader::datapackLoader->get_skins().size())
        ui->labelImageAvatar->setPixmap(getFrontSkin(playerInformations.public_informations.skinId));

    {
        QSqlQuery query;
        if(!query.prepare("SELECT * FROM note WHERE player = (:player)"))
            abort();
        query.bindValue(":player",pseudo);
        if(!query.exec())
        {
            qDebug() << "note load error: " << query.lastError();
            abort();
        }
        else if(query.next())
            ui->note->setPlainText(query.value("text").toString());
        updatePlayerKnownList(api);
        updateVisiblePlayers(api);
        update_chat();
    }

    {
        ui->listWidgetChatType->clear();
        ui->listWidgetChatType->addItem("local");
        ui->listWidgetChatType->addItem("clan");

        QSqlQuery query;
        if(!query.prepare("SELECT theotherplayer FROM privatechat WHERE player = (:player) GROUP BY theotherplayer;"))
            abort();
        query.bindValue(":player",pseudo);
        if(!query.exec())
        {
            qDebug() << "note load error: " << query.lastError();
            abort();
        }
        else
        {
            while(query.next())
                ui->listWidgetChatType->addItem("* "+query.value("theotherplayer").toString());
        }
    }
}

QPixmap SocialChat::getFrontSkin(const QString &skinName)
{
    if(frontSkinCacheString.contains(skinName))
        return frontSkinCacheString.value(skinName);
    const QPixmap skin(getFrontSkinPath(skinName));
    if(!skin.isNull())
        frontSkinCacheString[skinName]=skin;
    else
        frontSkinCacheString[skinName]=QPixmap(":/images/player_default/front.png");
    return frontSkinCacheString.value(skinName);
}

QPixmap SocialChat::getFrontSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)QtDatapackClientLoader::datapackLoader->get_skins().size())
        return getFrontSkin(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_skins().at(skinId)));
    else
        return getFrontSkin(QString());
}

QString SocialChat::getFrontSkinPath(const QString &skin)
{
    return getSkinPath(skin,"front");
}

QPixmap SocialChat::getBackSkin(const QString &skinName)
{
    if(backSkinCacheString.contains(skinName))
        return backSkinCacheString.value(skinName);
    const QPixmap skin(getBackSkinPath(skinName));
    if(!skin.isNull())
        backSkinCacheString[skinName]=skin;
    else
        backSkinCacheString[skinName]=QPixmap(":/images/player_default/back.png");
    return backSkinCacheString.value(skinName);
}

QPixmap SocialChat::getBackSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)QtDatapackClientLoader::datapackLoader->get_skins().size())
        return getFrontSkin(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_skins().at(skinId)));
    else
        return getFrontSkin(QString());
}

QString SocialChat::getBackSkinPath(const QString &skin)
{
    return getSkinPath(skin,"back");
}

QPixmap SocialChat::getTrainerSkin(const QString &skinName)
{
    if(trainerSkinCacheString.contains(skinName))
        return trainerSkinCacheString.value(skinName);
    const QPixmap skin(getTrainerSkinPath(skinName));
    if(!skin.isNull())
        trainerSkinCacheString[skinName]=skin;
    else
        trainerSkinCacheString[skinName]=QPixmap(":/images/player_default/trainer.png");
    return trainerSkinCacheString.value(skinName);
}

QPixmap SocialChat::getTrainerSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)QtDatapackClientLoader::datapackLoader->get_skins().size())
        return getTrainerSkin(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_skins().at(skinId)));
    else
        return getTrainerSkin(QString());
}

QString SocialChat::getTrainerSkinPath(const QString &skin)
{
    return getSkinPath(skin,"trainer");
}

QString SocialChat::getSkinPath(const QString &skinName,const QString &type)
{
    {
        QFileInfo pngFile(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_SKIN+skinName+QStringLiteral("/%1.png").arg(type));
        if(pngFile.exists())
            return pngFile.absoluteFilePath();
    }
    {
        QFileInfo gifFile(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_SKIN+skinName+QStringLiteral("/%1.gif").arg(type));
        if(gifFile.exists())
            return gifFile.absoluteFilePath();
    }
    QDir folderList(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_SKINBASE);
    const QStringList &entryList=folderList.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    int entryListIndex=0;
    while(entryListIndex<entryList.size())
    {
        {
            QFileInfo pngFile(QStringLiteral("%1/skin/%2/%3/%4.png").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())).arg(entryList.at(entryListIndex)).arg(skinName).arg(type));
            if(pngFile.exists())
                return pngFile.absoluteFilePath();
        }
        {
            QFileInfo gifFile(QStringLiteral("%1/skin/%2/%3/%4.gif").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())).arg(entryList.at(entryListIndex)).arg(skinName).arg(type));
            if(gifFile.exists())
                return gifFile.absoluteFilePath();
        }
        entryListIndex++;
    }
    return QString();
}

void SocialChat::on_note_textChanged()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    {
        QSqlQuery query;
        if(!query.prepare("DELETE FROM note WHERE player = (:player)"))
            abort();
        query.bindValue(":player", pseudo);
        if(!query.exec())
        {
            qDebug() << "on_note_textChanged del error:  " << query.lastError();
            abort();
        }
    }
    {
        QSqlQuery query;
        if(!query.prepare("INSERT INTO note (player,text) VALUES (:player,:text)"))
            abort();
        query.bindValue(":player", pseudo);
        query.bindValue(":text", ui->note->toPlainText());
        if(!query.exec())
        {
            qDebug() << "on_note_textChanged add error:  " << query.lastError();
            abort();
        }
    }
}

void SocialChat::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,
                               const std::string &theotherpseudo,const CatchChallenger::Player_type &player_type)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(api==NULL)
        return;
    const QString pseudo=QString::fromStdString(api->getPseudo());

    if(chat_type==CatchChallenger::Chat_type::Chat_type_all)
        if(pseudoToBot.contains(pseudo))
            return;

    new_chat_text_internal(chat_type,QString::fromStdString(text),pseudo,QString::fromStdString(theotherpseudo),player_type);
}

void SocialChat::new_chat_text_internal(const CatchChallenger::Chat_type &chat_type,const QString &text,
                                        const QString &pseudo,//what bot receive this text
                                        const QString &theotherpseudo,//what is the player sending this text (can be the bot him self)
                                        const CatchChallenger::Player_type &player_type)
{
    if(chat_type==CatchChallenger::Chat_type::Chat_type_all)
        knownGlobalChatPlayers << pseudo;

    ChatEntry newEntry;
    newEntry.player_type=player_type;
    newEntry.player_pseudo=theotherpseudo.toStdString();
    newEntry.chat_type=chat_type;
    newEntry.text=text.toStdString();

    for (const auto &n:ActionsBotInterface::clientList) {
        const ActionsBotInterface::Player &client=n.second;
        bool found=false;
        if(chat_type!=CatchChallenger::Chat_type::Chat_type_pm)
            found=text.contains(client.regexMatchPseudo);
        else
        {
            unsigned int index=2;
            while(index<(unsigned int)ui->listWidgetChatType->count())
            {
                QListWidgetItem * item=ui->listWidgetChatType->item(index);
                QString text=item->text();
                if(!text.startsWith("* "))
                    abort();
                text.remove(0,2);
                if(text==pseudo)
                {
                    item->setBackground(QBrush(QColor("#DDDDFF"),Qt::SolidPattern));
                    break;
                }
                index++;
            }
            //new entry
            if(index>=(unsigned int)ui->listWidgetChatType->count())
            {
                ui->listWidgetChatType->addItem("* "+pseudo);
                QListWidgetItem * item=ui->listWidgetChatType->item(ui->listWidgetChatType->count()-1);
                item->setBackground(QBrush(QColor("#DDDDFF"),Qt::SolidPattern));
            }
            found=false;
        }
        if(found)
        {
            //mark the local or clan as unread
/*            if(item->text()==client.api->getPseudo())
            {
                client.viewedPlayers << pseudo;
                if(item->isElect() && ui->listWidgetChatType->count()>1)
                {
                    if(chat_type==CatchChallenger::Chat_type::Chat_type_local)
                        ui->listWidgetChatType->item(0)->setBackground(QBrush(QColor("#DDDDFF"),Qt::SolidPattern));
                    if(chat_type==CatchChallenger::Chat_type::Chat_type_clan)
                        ui->listWidgetChatType->item(1)->setBackground(QBrush(QColor("#DDDDFF"),Qt::SolidPattern));
                }
            }*/
            if(!hasFocus())
            {
                if(!windowTitle().endsWith("*"))
                    setWindowTitle(windowTitle()+"*");
            }
            else
            {
                const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
                if(selectedItems.size()!=1)
                    return;
                const QString &pseudo=selectedItems.at(0)->text();
                if(!pseudoToBot.contains(pseudo))
                    return;

                if(pseudo!=QString::fromStdString(client.api->getPseudo()))
                {
                    if(!windowTitle().endsWith("*"))
                        setWindowTitle(windowTitle()+"*");
                }
            }
            unsigned int index=0;
            while(index<(uint32_t)ui->bots->count())
            {
                QListWidgetItem * const item=ui->bots->item(index);
                if(item->text()==QString::fromStdString(client.api->getPseudo()) && (!hasFocus() || !item->isSelected()))
                    item->setBackground(QBrush(QColor("#DDDDFF"),Qt::SolidPattern));
                index++;
            }
        }
    }

    //drop duplicate message
    if(chat_type==CatchChallenger::Chat_type::Chat_type_all)
    {
        const std::string tempContent=newEntry.player_pseudo+newEntry.text;
        {
            unsigned int index=0;
            while(index<lastText.size())
            {
                const std::unordered_set<std::string> &searchBlock=lastText.at(index);
                if(searchBlock.find(tempContent)!=searchBlock.cend())
                    return;
                index++;
            }
        }
        lastText.back().insert(tempContent);
    }

    {
        QSqlQuery query;
        switch(chat_type)
        {
            case CatchChallenger::Chat_type::Chat_type_all:
            if(!query.prepare("INSERT INTO globalchat (chat_type,text,pseudo,player_type) VALUES (:chat_type,:text,:pseudo,:player_type)"))
                abort();
            query.bindValue(":chat_type", (uint8_t)newEntry.chat_type);
            query.bindValue(":text", QString::fromStdString(newEntry.text));
            query.bindValue(":pseudo", QString::fromStdString(newEntry.player_pseudo));
            query.bindValue(":player_type", (uint8_t)newEntry.player_type);
            if(!query.exec())
            {
                qDebug() << "on_note_textChanged add error:  " << query.lastError();
                abort();
            }
            chat_list << newEntry;
            while(chat_list.size()>64)
                chat_list.removeFirst();
            update_chat();
            break;
            case CatchChallenger::Chat_type::Chat_type_local:
            case CatchChallenger::Chat_type::Chat_type_clan:
            {
                if(!query.prepare("INSERT INTO otherchat (chat_type,text,player_type,player,theotherplayer) VALUES (:chat_type,:text,:player_type,:player,:theotherplayer)"))
                    abort();
                query.bindValue(":chat_type", (uint8_t)newEntry.chat_type);
                query.bindValue(":text", QString::fromStdString(newEntry.text));
                query.bindValue(":player", pseudo);
                query.bindValue(":theotherplayer", QString::fromStdString(newEntry.player_pseudo));
                query.bindValue(":player_type", (uint8_t)newEntry.player_type);
                if(!query.exec())
                {
                    qDebug() << "on_note_textChanged add error:  " << query.lastError();
                    abort();
                }
                switch(chat_type)
                {
                    case CatchChallenger::Chat_type::Chat_type_local:
                        if(ui->listWidgetChatType->item(0)->isSelected())
                            on_listWidgetChatType_itemSelectionChanged();
                    break;
                    case CatchChallenger::Chat_type::Chat_type_clan:
                        if(ui->listWidgetChatType->item(1)->isSelected())
                            on_listWidgetChatType_itemSelectionChanged();
                    break;
                    default:
                    break;
                }
            }
            break;
            case CatchChallenger::Chat_type::Chat_type_pm:
            {
                if(!query.prepare("INSERT INTO privatechat (text,player_type,player,theotherplayer,fromplayer) VALUES (:text,:player_type,:player,:theotherplayer,:fromplayer)"))
                    abort();
                query.bindValue(":text", QString::fromStdString(newEntry.text));
                query.bindValue(":player", pseudo);
                query.bindValue(":theotherplayer", QString::fromStdString(newEntry.player_pseudo));
                query.bindValue(":player_type", (uint8_t)newEntry.player_type);
                query.bindValue(":fromplayer",0);
                if(!query.exec())
                {
                    qDebug() << "on_note_textChanged add error:  " << query.lastError();
                    abort();
                }
                const QList<QListWidgetItem*> &selectedItemsType=ui->listWidgetChatType->selectedItems();
                if(selectedItemsType.size()!=1)
                    return;
                if(selectedItemsType.at(0)->text()==QString::fromStdString("* "+newEntry.player_pseudo))
                    on_listWidgetChatType_itemSelectionChanged();
            }
            break;
            default:
            break;
        }
    }
    /*{
        QSqlQuery query;
        if(!query.prepare("DELETE FROM globalchat WHERE player = (:player)"))
        abort();
        query.bindValue(":player", pseudo);
        if(!query.exec())
            qDebug() << "on_note_textChanged del error:  " << query.lastError();
    }*/
}

void SocialChat::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    ChatEntry newEntry;
    newEntry.player_type=CatchChallenger::Player_type_normal;
    //newEntry.player_pseudo=QString();
    newEntry.chat_type=chat_type;
    newEntry.text=text;

    const std::string tempContent=newEntry.text;
    {
        unsigned int index=0;
        while(index<lastText.size())
        {
            const std::unordered_set<std::string> &searchBlock=lastText.at(index);
            if(searchBlock.find(tempContent)!=searchBlock.cend())
                return;
            index++;
        }
    }
    lastText.back().insert(tempContent);

    chat_list << newEntry;
    while(chat_list.size()>64)
        chat_list.removeFirst();
    //update_chat();
}

void SocialChat::update_chat()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * api=pseudoToBot.value(pseudo);
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    const ActionsBotInterface::Player &client=ActionsBotInterface::clientList.at(api);

    QString nameHtml;
    int index=0;
    while(index<chat_list.size())
    {
        const ChatEntry &entry=chat_list.at(index);
        bool addPlayerInfo=true;
        if(entry.chat_type==CatchChallenger::Chat_type_system || entry.chat_type==CatchChallenger::Chat_type_system_important)
            addPlayerInfo=false;
        if(!addPlayerInfo)
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(std::string(),CatchChallenger::Player_type_normal,entry.chat_type,entry.text));
        else
        {
            const QString &QtText=QString::fromStdString(entry.text);
            bool found=QtText.contains(client.regexMatchPseudo);
            if(found)
                nameHtml+="<div style=\"background-color:#DDDDFF;\">";
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(entry.player_pseudo,entry.player_type,entry.chat_type,entry.text,true));
            if(found)
                nameHtml+="</div>";
        }
        index++;
    }
    ui->globalChat->setHtml(nameHtml);
    ui->globalChat->verticalScrollBar()->setValue(ui->globalChat->verticalScrollBar()->maximum());
}

void SocialChat::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;

    lastText.push_back(std::unordered_set<std::string>());
    if(lastText.size()>6)//6*1.5s=9s
        lastText.erase(lastText.cbegin()+lastText.size()-1);
}

void SocialChat::on_globalChatText_returnPressed()
{
    QString text=ui->globalChatText->text();
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(windowTitle().endsWith("*"))
    {
        QString title=windowTitle();
        title.remove("*");
        setWindowTitle(title);
    }
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * api=pseudoToBot.value(pseudo);
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;

    selectedItems.at(0)->setBackground(Qt::NoBrush);
    if(text.isEmpty())
        return;
    if(text.contains(QRegularExpression("^ +$")))
    {
        ui->globalChatText->clear();
        new_system_text(CatchChallenger::Chat_type_system,"Space text not allowed");
        return;
    }
    if(text.size()>256)
    {
        ui->globalChatText->clear();
        new_system_text(CatchChallenger::Chat_type_system,"Message too long");
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text==lastMessageSend)
        {
            ui->globalChatText->clear();
            new_system_text(CatchChallenger::Chat_type_system,"Send message like as previous");
            return;
        }
        if(numberForFlood>2)
        {
            ui->globalChatText->clear();
            new_system_text(CatchChallenger::Chat_type_system,"Stop flood");
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text;

    if(!text.startsWith("/pm "))
    {
        ui->globalChat->setText(QString());
        if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
            return;
        const CatchChallenger::Player_private_and_public_informations &player_informations=api->get_player_informations();

        api->sendChatText(CatchChallenger::Chat_type_all,text.toStdString());
        if(!text.startsWith('/'))
            new_chat_text_internal(CatchChallenger::Chat_type_all,text,pseudo,pseudo,player_informations.public_informations.type);
        ui->globalChatText->clear();
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        QString message=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        message.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        ui->globalChatText->clear();
        unsigned int index=0;
        while(index<(unsigned int)ui->listWidgetChatType->count())
        {
            ui->listWidgetChatType->item(index)->setSelected(false);
            index++;
        }
        index=2;
        while(index<(unsigned int)ui->listWidgetChatType->count())
        {
            QListWidgetItem * item=ui->listWidgetChatType->item(index);
            QString text=item->text();
            if(text.startsWith("* "))
            {
                text.remove(0,2);
                if(text==pseudo)
                {
                    ui->listWidgetChatType->item(index)->setSelected(true);
                    on_listWidgetChatType_itemSelectionChanged();
                    ui->chatSpecText->setText(message);
                    on_chatSpecText_returnPressed();
                    return;
                }
            }
            index++;
        }
        //if(index>=ui->listWidgetChatType->count())//implicit
        {
            ui->listWidgetChatType->addItem("* "+pseudo);
            ui->listWidgetChatType->item(ui->listWidgetChatType->count()-1)->setSelected(true);
            on_listWidgetChatType_itemSelectionChanged();
            ui->chatSpecText->setText(message);
            on_chatSpecText_returnPressed();
            return;
        }
        return;
    }
}

void SocialChat::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    (void)player;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(api==NULL)
        return;
    updatePlayerKnownList(api);
    updateVisiblePlayers(api);
}

void SocialChat::remove_player(const uint16_t &id)
{
    (void)id;
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(api==NULL)
        return;
    updateVisiblePlayers(api);
}

void SocialChat::dropAllPlayerOnTheMap()
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(QObject::sender());
    if(api==NULL)
        return;
    updateVisiblePlayers(api);
}

void SocialChat::updatePlayerKnownList(CatchChallenger::Api_protocol_Qt *api)
{
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    if(api->getPseudo()==pseudo.toStdString())
        globalChatText_updateCompleter();
}

void SocialChat::updateVisiblePlayers(CatchChallenger::Api_protocol_Qt *api)
{
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    if(api->getPseudo()==pseudo.toStdString())
    {
        QString newHtml;
        for (const auto &n:ActionsBotInterface::clientList[api].visiblePlayers) {
            const CatchChallenger::Player_public_informations &playerInformations=n.second;
            if(!newHtml.isEmpty())
                newHtml+=", ";

            newHtml+="<div style=\"";
            switch(playerInformations.type)
            {
                case CatchChallenger::Player_type_normal://normal player
                break;
                case CatchChallenger::Player_type_premium://premium player
                break;
                case CatchChallenger::Player_type_gm://gm
                    newHtml+="font-weight:bold;";
                break;
                case CatchChallenger::Player_type_dev://dev
                    newHtml+="font-weight:bold;";
                break;
                default:
                break;
            }
            newHtml+="\">";
            switch(playerInformations.type)
            {
                case CatchChallenger::Player_type_normal://normal player
                break;
                case CatchChallenger::Player_type_premium://premium player
                    newHtml+="<img src=\":/images/chat/premium.png\" alt\"\" />";
                break;
                case CatchChallenger::Player_type_gm://gm
                    newHtml+="<img src=\":/images/chat/admin.png\" alt\"\" />";
                break;
                case CatchChallenger::Player_type_dev://dev
                    newHtml+="<img src=\":/images/chat/developer.png\" alt\"\" />";
                break;
                default:
                break;
            }

            newHtml+=QString::fromStdString(playerInformations.pseudo);

            newHtml+="</div>";
        }
        ui->labelVisiblePlayer->setText(newHtml);
    }
}

void SocialChat::on_globalChat_anchorClicked(const QUrl &arg1)
{
    ui->globalChatText->insert(" @"+arg1.toString()+" ");//ui->globalChatText->cursorPosition(),
    ui->globalChatText->setFocus();
}

void SocialChat::globalChatText_updateCompleter()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * const api=pseudoToBot.value(pseudo);

    QSet<QString> viewedPlayers;
    for(const auto &n:ActionsBotInterface::clientList[api].viewedPlayers)
        viewedPlayers.insert(n);
    QList<QString> wordList=QSet<QString>(knownGlobalChatPlayers+viewedPlayers).toList();
    if(completer!=NULL)
    {
        delete completer;
        completer=NULL;
    }
    const int cursorPosition=ui->globalChatText->cursorPosition();
    if(cursorPosition<0)
        completer = new QCompleter(wordList);
    else
    {
        const QString &fullText=ui->globalChatText->text();
        const QString &beforeTheCursor=fullText.mid(0,cursorPosition);
        const int &spliterPos=beforeTheCursor.lastIndexOf("@");
        if(spliterPos<0)
            completer = new QCompleter(wordList);
        else
        {
            QList<QString> finalWordList;
            const QString &pos1=beforeTheCursor.mid(0,spliterPos+1);
            const QString &pos2=beforeTheCursor.mid(spliterPos+1);
            const QString &pos3=fullText.mid(cursorPosition);
            if(!pos2.isEmpty())
            {
                unsigned int index=0;
                while(index<(uint32_t)wordList.size())
                {
                    const QString &pseudo=wordList.at(index);
                    if(pseudo.startsWith(pos2,Qt::CaseInsensitive))
                        finalWordList << pos1+pseudo+pos3;
                    index++;
                }
            }
            else
            {
                unsigned int index=0;
                while(index<(uint32_t)wordList.size())
                {
                    finalWordList << pos1+wordList.at(index)+pos3;
                    index++;
                }
            }
            completer = new QCompleter(wordList);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
            qDebug() << "finalWordList: " << finalWordList.join("\n") << completer->completionPrefix() << " end";
        }
    }


    //completer->setCaseSensitivity(Qt::CaseInsensitive);
    //completer->setCompletionMode(QCompleter::InlineCompletion);
    ui->globalChatText->setCompleter(completer);
}

void SocialChat::on_chatSpecText_returnPressed()
{
    QString text=ui->chatSpecText->text();
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(windowTitle().endsWith("*"))
    {
        QString title=windowTitle();
        title.remove("*");
        setWindowTitle(title);
    }
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * api=pseudoToBot.value(pseudo);
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;

    selectedItems.at(0)->setBackground(Qt::NoBrush);

    const QList<QListWidgetItem*> &selectedItemsType=ui->listWidgetChatType->selectedItems();
    if(selectedItemsType.size()!=1)
        return;
    selectedItemsType.at(0)->setBackground(Qt::NoBrush);

    if(text.isEmpty())
        return;
    if(text.contains(QRegularExpression("^ +$")))
    {
        ui->chatSpecText->clear();
        new_system_text(CatchChallenger::Chat_type_system,"Space text not allowed");
        std::cerr << pseudo.toStdString() << ": Space text not allowed" << std::endl;
        return;
    }
    if(text.size()>256)
    {
        ui->chatSpecText->clear();
        new_system_text(CatchChallenger::Chat_type_system,"Message too long");
        std::cerr << pseudo.toStdString() << ": Message too long" << std::endl;
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text==lastMessageSend)
        {
            ui->chatSpecText->clear();
            new_system_text(CatchChallenger::Chat_type_system,"Send message like as previous");
            std::cerr << pseudo.toStdString() << ": Send message like as previous" << std::endl;
            return;
        }
        if(numberForFlood>2)
        {
            ui->chatSpecText->clear();
            new_system_text(CatchChallenger::Chat_type_system,"Stop flood");
            std::cerr << pseudo.toStdString() << ": Stop flood (" << std::to_string(numberForFlood) << ")" << std::endl;
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text;

    if(!text.startsWith("/pm "))
    {
        ui->chatSpec->setText(QString());
        if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
            return;
        const CatchChallenger::Player_private_and_public_informations &player_informations=api->get_player_informations();

        QString textSelectionChatType=selectedItemsType.at(0)->text();
        CatchChallenger::Chat_type chatType=CatchChallenger::Chat_type_pm;
        if(textSelectionChatType=="local")
            chatType=CatchChallenger::Chat_type_local;
        else if(textSelectionChatType=="clan")
            chatType=CatchChallenger::Chat_type_clan;
        switch(chatType)
        {
            case CatchChallenger::Chat_type_local:
            case CatchChallenger::Chat_type_clan:
                api->sendChatText(chatType,text.toStdString());
                if(!text.startsWith('/'))
                    new_chat_text_internal(chatType,text,pseudo,pseudo,player_informations.public_informations.type);
                ui->chatSpecText->clear();
            break;
            default:
            {
                if(!textSelectionChatType.startsWith("* "))
                    abort();
                textSelectionChatType.remove(0,2);
                api->sendPM(text.toStdString(),textSelectionChatType.toStdString());
                if(!text.startsWith('/'))
                {
                    QSqlQuery query;
                    if(!query.prepare("INSERT INTO privatechat (text,player_type,player,theotherplayer,fromplayer) VALUES (:text,:player_type,:player,:theotherplayer,:fromplayer)"))
                        abort();
                    query.bindValue(":text", text);
                    query.bindValue(":player", QString::fromStdString(api->getPseudo()));
                    query.bindValue(":theotherplayer", textSelectionChatType);
                    query.bindValue(":player_type", (uint8_t) api->get_player_informations().public_informations.type);
                    query.bindValue(":fromplayer",1);
                    if(!query.exec())
                    {
                        qDebug() << "note load error: " << query.lastError();
                        abort();
                    }
                    on_listWidgetChatType_itemSelectionChanged();
                }
                ui->chatSpecText->clear();
            }
            break;
        }
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        QString message=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        message.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        ui->chatSpecText->clear();
        unsigned int index=0;
        while(index<(unsigned int)ui->listWidgetChatType->count())
        {
            ui->listWidgetChatType->item(index)->setSelected(false);
            index++;
        }
        index=0;
        while(index<(unsigned int)ui->listWidgetChatType->count())
        {
            QListWidgetItem * item=ui->listWidgetChatType->item(index);
            QString text=item->text();
            if(text.startsWith("* "))
            {
                text.remove(0,2);
                if(text==pseudo)
                {
                    ui->listWidgetChatType->item(index)->setSelected(true);
                    on_listWidgetChatType_itemSelectionChanged();
                    ui->chatSpecText->setText(message);
                    on_chatSpecText_returnPressed();
                    return;
                }
            }
            index++;
        }
        //if(index>=ui->listWidgetChatType->count())//implicit
        {
            ui->listWidgetChatType->addItem("* "+pseudo);
            ui->listWidgetChatType->item(ui->listWidgetChatType->count()-1)->setSelected(true);
            on_listWidgetChatType_itemSelectionChanged();
            ui->chatSpecText->setText(message);
            on_chatSpecText_returnPressed();
            return;
        }
        return;
    }
}

void SocialChat::on_listWidgetChatType_itemSelectionChanged()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    CatchChallenger::Api_protocol_Qt * api=pseudoToBot.value(pseudo);
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    const ActionsBotInterface::Player &client=ActionsBotInterface::clientList.at(api);

    if(ui->listWidgetChatType->count()<=1)
        return;
    if(ui->listWidgetChatType->item(0)->isSelected())
    {
        ui->listWidgetChatType->item(1)->setBackground(Qt::NoBrush);
        QSqlQuery query;
        if(!query.prepare("SELECT * FROM otherchat WHERE player = (:player) AND chat_type = (:chat_type)"))
            abort();
        query.bindValue(":player",pseudo);
        query.bindValue(":chat_type",CatchChallenger::Chat_type_local);
        if(!query.exec())
        {
            qDebug() << "note load error: " << query.lastError();
            abort();
        }
        else
        {
            QString nameHtml;
            while(query.next())
            {
                const QString &QtText=query.value("text").toString();
                bool found=QtText.contains(client.regexMatchPseudo);
                if(found)
                    nameHtml+="<div style=\"background-color:#DDDDFF;\">";
                nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
                                                     query.value("theotherplayer").toString().toStdString(),
                                                     (CatchChallenger::Player_type)query.value("player_type").toUInt(),
                                                     CatchChallenger::Chat_type_local,
                                                     QtText.toStdString(),
                                                     true));
                if(found)
                    nameHtml+="</div>";
            }
            ui->chatSpec->setHtml(nameHtml);
            ui->chatSpec->verticalScrollBar()->setValue(ui->chatSpec->verticalScrollBar()->maximum());
        }
    }
    else if(ui->listWidgetChatType->item(1)->isSelected())
    {
        ui->listWidgetChatType->item(1)->setBackground(Qt::NoBrush);
        QSqlQuery query;
        if(!query.prepare("SELECT * FROM otherchat WHERE player = (:player) AND chat_type = (:chat_type)"))
            abort();
        query.bindValue(":player",pseudo);
        query.bindValue(":chat_type",CatchChallenger::Chat_type_clan);
        if(!query.exec())
        {
            qDebug() << "note load error: " << query.lastError();
            abort();
        }
        else
        {
            QString nameHtml;
            while(query.next())
            {
                const QString &QtText=query.value("text").toString();
                bool found=QtText.contains(client.regexMatchPseudo);
                if(found)
                    nameHtml+="<div style=\"background-color:#DDDDFF;\">";
                nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
                                                     query.value("theotherplayer").toString().toStdString(),
                                                     (CatchChallenger::Player_type)query.value("player_type").toUInt(),
                                                     CatchChallenger::Chat_type_clan,
                                                     QtText.toStdString(),
                                                     true));
                if(found)
                    nameHtml+="</div>";
            }
            ui->chatSpec->setHtml(nameHtml);
            ui->chatSpec->verticalScrollBar()->setValue(ui->chatSpec->verticalScrollBar()->maximum());
        }
    }
    else
    {
        unsigned int index=2;
        while(index<(unsigned int)ui->listWidgetChatType->count())
        {
            QListWidgetItem * item=ui->listWidgetChatType->item(index);
            if(item->isSelected())
            {
                QString text=item->text();
                if(!text.startsWith("* "))
                    abort();
                text.remove(0,2);

                const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
                QSqlQuery query;
                if(!query.prepare("SELECT * FROM privatechat WHERE player = (:player) AND theotherplayer = (:theotherplayer)"))
                    abort();
                query.bindValue(":player",pseudo);
                query.bindValue(":theotherplayer",text);
                if(!query.exec())
                {
                    qDebug() << "note load error: " << query.lastError();
                    abort();
                }
                else
                {
                    QString nameHtml;
                    while(query.next())
                    {
                        QString QtText=query.value("text").toString();
                        if(query.value("fromplayer").toUInt()==0)
                            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
                                                                 query.value("theotherplayer").toString().toStdString(),
                                                                 (CatchChallenger::Player_type)query.value("player_type").toUInt(),
                                                                 CatchChallenger::Chat_type_pm,
                                                                 QtText.toStdString(),
                                                                 true));
                        else
                            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
                                                                 playerInformations.public_informations.pseudo,
                                                                 playerInformations.public_informations.type,
                                                                 CatchChallenger::Chat_type_pm,
                                                                 QtText.toStdString(),
                                                                 true));

                    }
                    ui->chatSpec->setHtml(nameHtml);
                    ui->chatSpec->verticalScrollBar()->setValue(ui->chatSpec->verticalScrollBar()->maximum());
                }
                item->setBackground(Qt::NoBrush);
                break;
            }
            index++;
        }
    }
}
