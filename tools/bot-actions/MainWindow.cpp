#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../bot/actions/ActionsAction.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../client/libqtcatchchallenger/Settings.hpp"

#include <QNetworkProxy>
#include <QMessageBox>

MultipleBotConnectionAction MainWindow::multipleBotConnexion;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Settings::settings=new QSettings();
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    internalId=0;
    srand(time(0));
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<QList<QList<CatchChallenger::CharacterEntry> > >("QList<QList<CatchChallenger::CharacterEntry> >");
    qRegisterMetaType<QList<CatchChallenger::ServerFromPoolForDisplay*> >("QList<CatchChallenger::ServerFromPoolForDisplay*>");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");

    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");

    qRegisterMetaType<CatchChallenger::Chat_type>("Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("Player_private_and_public_informations");

    qRegisterMetaType<QHash<uint32_t,uint32_t> >("QHash<uint32_t,uint32_t>");
    qRegisterMetaType<QHash<uint32_t,uint32_t> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<QList<CatchChallenger::ItemToSellOrBuy> >("QList<ItemToSell>");
    qRegisterMetaType<QList<QPair<uint8_t,uint8_t> > >("QList<QPair<uint8_t,uint8_t> >");
    qRegisterMetaType<CatchChallenger::Skill::AttackReturn>("Skill::AttackReturn");
    qRegisterMetaType<QList<uint32_t> >("QList<uint32_t>");
    qRegisterMetaType<QList<QList<CatchChallenger::CharacterEntry> > >("QList<QList<CharacterEntry> >");
    qRegisterMetaType<std::vector<CatchChallenger::MarketMonster> >("std::vector<MarketMonster>");

    qRegisterMetaType<std::unordered_map<uint16_t,uint16_t> >("std::unordered_map<uint16_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint16_t,uint32_t> >("std::unordered_map<uint16_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint16_t> >("std::unordered_map<uint8_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");

    qRegisterMetaType<std::map<uint16_t,uint16_t> >("std::map<uint16_t,uint16_t>");
    qRegisterMetaType<std::map<uint16_t,uint32_t> >("std::map<uint16_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint16_t> >("std::map<uint8_t,uint16_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<std::vector<CatchChallenger::ServerFromPoolForDisplay> >("std::vector<CatchChallenger::ServerFromPoolForDisplay>");
    qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry> > >("std::vector<std::vector<CatchChallenger::CharacterEntry> >");

    ui->setupUi(this);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::loggedDone,this,&MainWindow::logged,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackIsReady,this,&MainWindow::datapackIsReady,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackMainSubIsReady,this,&MainWindow::datapackMainSubIsReady,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::statusError,this,&MainWindow::statusError,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfSelectedCharacter,this,&MainWindow::display_numberOfSelectedCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfStartSelectingCharacter,this,&MainWindow::display_numberOfStartSelectingCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfHaveDatapackCharacter,this,&MainWindow::display_numberOfHaveDatapackCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfStartCreatingCharacter,this,&MainWindow::display_numberOfStartCreatingCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfStartCreatedCharacter,this,&MainWindow::display_numberOfStartCreatedCharacter,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::updateClientListStatus,this,&MainWindow::updateClientListStatus,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfBotConnected,this,&MainWindow::display_numberOfBotConnected))
        abort();//,Qt::QueuedConnection
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_detectSlowDown,this,&MainWindow::detectSlowDown,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_connected,this,&MainWindow::all_player_connected,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_on_map,this,&MainWindow::all_player_on_map,Qt::QueuedConnection))
        abort();
    if(!connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&MainWindow::lastReplyTime,Qt::QueuedConnection))
        abort();
    if(!connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::detectSlowDown,Qt::QueuedConnection))
        abort();
    slowDownTimer.start(200);

    multipleBotConnexion.botInterface=new ActionsAction();

    if(settings.contains("login"))
        ui->login->setText(settings.value("login").toString());
    if(settings.contains("pass"))
        ui->pass->setText(settings.value("pass").toString());
    if(settings.contains("host"))
        ui->host->setText(settings.value("host").toString());
    if(settings.contains("port"))
        ui->port->setValue(settings.value("port").toUInt());
    if(settings.contains("proxy"))
        ui->proxy->setText(settings.value("proxy").toString());
    if(settings.contains("proxyport"))
        ui->proxyport->setValue(settings.value("proxyport").toUInt());
    if(settings.contains("multipleConnexion"))
    {
        if(settings.value("multipleConnexion").toUInt()<2)
            ui->multipleConnexion->setChecked(false);
        else
        {
            ui->multipleConnexion->setChecked(true);
            const unsigned int multipleConnexion=settings.value("multipleConnexion").toUInt();
            ui->connexionCountTarget->setValue(multipleConnexion);
        }
        if(settings.contains("connectBySeconds"))
            ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
        if(settings.contains("maxDiffConnectedSelected"))
            ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());
    }
    if(settings.contains("autoCreateCharacter"))
        ui->autoCreateCharacter->setChecked(settings.value("autoCreateCharacter").toBool());
    ui->serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
    ui->serverList->header()->resizeSection(0,680);
    ui->groupBox_char->setEnabled(false);
    ui->groupBox_Server->setEnabled(false);

    //LanguagesSelect::languagesSelect=new LanguagesSelect();
    botTargetList=NULL;
}

MainWindow::~MainWindow()
{
    delete multipleBotConnexion.botInterface;
    delete ui;
}

void MainWindow::lastReplyTime(const quint32 &time)
{
    ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
}


void MainWindow::detectSlowDown(uint32_t queryCount,uint32_t worseTime)
{
    if(worseTime>3600*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2h").arg(queryCount).arg(worseTime/(3600*1000)));
    else if(worseTime>2*60*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2min").arg(queryCount).arg(worseTime/(60*1000)));
    else if(worseTime>5*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2s").arg(queryCount).arg(worseTime/(1000)));
    else
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2ms").arg(queryCount).arg(worseTime));
}

void MainWindow::logged(CatchChallenger::Api_client_real *senderObject,
                        const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList,
                        const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList,
                        bool haveTheDatapack)
{
    Q_UNUSED(haveTheDatapack);
    CatchChallenger::Api_client_real * api=senderObject;
    if(api==NULL)
    {
        qDebug() << "MainWindow::logged(): qobject_cast<CatchChallenger::Api_client_real *>(sender())==NULL";
        return;
    }

    ui->characterList->clear();
    this->serverOrdenedList=serverOrdenedList;
    this->characterEntryList=characterEntryList;

    ui->characterList->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());

    ui->serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
    ui->serverList->header()->resizeSection(0,400);
    updateServerList(api);

    ActionsAction *actionsAction=static_cast<ActionsAction *>(multipleBotConnexion.botInterface);
    if(ActionsAction::clientList.find(api)==ActionsAction::clientList.cend())
    {
        ActionsBotInterface::Player newPlayer;
        //newPlayer.player=0;
        newPlayer.mapId=0;
        newPlayer.internalId=internalId++;
        newPlayer.x=0;
        newPlayer.y=0;
        newPlayer.canMoveOnMap=false;
        newPlayer.repel_step=0;
        newPlayer.target.blockObject=NULL;
        newPlayer.target.extra=0;
        newPlayer.target.linkPoint.x=0;
        newPlayer.target.linkPoint.y=0;
        newPlayer.target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
        newPlayer.target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
        newPlayer.target.wildCycle=0;
        newPlayer.api=api;
        ActionsAction::clientList[api]=newPlayer;

        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qthave_inventory,     actionsAction,&ActionsAction::have_inventory_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtadd_to_inventory,   actionsAction,&ActionsAction::add_to_inventory_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtremove_to_inventory,actionsAction,&ActionsAction::remove_to_inventory_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::QtsetEvents,   actionsAction,&ActionsAction::setEvents_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::QtnewEvent,actionsAction,&ActionsAction::newEvent_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtrandom_seeds,actionsAction,&ActionsAction::newRandomNumber_slot,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::QtmonsterCatch,actionsAction,&ActionsAction::monsterCatch))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::QtteleportTo,actionsAction,&ActionsAction::teleportTo))
            abort();
    }
    else
        abort();
}

void MainWindow::updateServerList(CatchChallenger::Api_client_real *senderObject)
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        unsigned int index=0;
        int serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
            if(serverByCharacterGroup.contains(server.charactersGroupIndex))
                serverByCharacterGroup[server.charactersGroupIndex].first++;
            else
            {
                serverByCharacterGroup[server.charactersGroupIndex].first=1;
                serverByCharacterGroup[server.charactersGroupIndex].second=serverByCharacterGroupTempIndexToDisplay;
                serverByCharacterGroupTempIndexToDisplay++;
            }
            index++;
        }
    }

    //clear and determine what kind of view
    ui->serverList->clear();

    CatchChallenger::LogicialGroup logicialGroup=senderObject->getLogicialGroup();
    bool fullView=true;
    if(serverOrdenedList.size()>10)
        fullView=false;
    const uint64_t &current__date=QDateTime::currentDateTime().toTime_t();

    addToServerList(logicialGroup,ui->serverList->invisibleRootItem(),current__date,fullView);
    ui->serverList->expandAll();
}

void MainWindow::statusError(QString error)
{
    ui->statusBar->showMessage(error);
}

void MainWindow::on_connect_clicked()
{
    if(!ui->connect->isEnabled())
        return;
    if(ui->pass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    if(ui->login->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    ui->groupBox_MultipleConnexion->setEnabled(false);
    ui->groupBox_Proxy->setEnabled(false);
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());
    if(ui->multipleConnexion->isChecked())
        settings.setValue("multipleConnexion",ui->connexionCountTarget->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());

    multipleBotConnexion.mLogin=ui->login->text();
    multipleBotConnexion.mPass=ui->pass->text();
    multipleBotConnexion.mMultipleConnexion=ui->multipleConnexion->isChecked();
    multipleBotConnexion.mAutoCreateCharacter=ui->autoCreateCharacter->isChecked();
    multipleBotConnexion.mConnectBySeconds=ui->connectBySeconds->value();
    multipleBotConnexion.mConnexionCountTarget=ui->connexionCountTarget->value();
    multipleBotConnexion.mMaxDiffConnectedSelected=ui->maxDiffConnectedSelected->value();
    multipleBotConnexion.mProxy=ui->proxy->text();
    multipleBotConnexion.mProxyport=ui->proxyport->value();
    multipleBotConnexion.mHost=ui->host->text();
    multipleBotConnexion.mPort=ui->port->value();

    if(ui->autoCreateCharacter->isChecked())
    {
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
    }
    if(ui->multipleConnexion->isChecked())
    {
        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);
    }
    ui->connect->setEnabled(false);

    //do only the first client to download the datapack
    multipleBotConnexion.createClient();
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    multipleBotConnexion.characterSelectForFirstCharacter(ui->characterList->currentData().toUInt());
    ui->groupBox_char->setEnabled(false);
}

void MainWindow::display_numberOfBotConnected(quint16 numberOfBotConnected)
{
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));
}

void MainWindow::display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter)
{
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));
}

void MainWindow::display_numberOfStartSelectingCharacter(quint16 numberOfStartSelectingCharacter)
{
    ui->numberOfSelectingCharacter->setText(tr("Selecting character: %1").arg(numberOfStartSelectingCharacter));
}

void MainWindow::display_numberOfHaveDatapackCharacter(quint16 numberOfHaveDatapackCharacter)
{
    ui->numberOfHaveDatapackCharacter->setText(tr("numberOfHaveDatapackCharacter: %1").arg(numberOfHaveDatapackCharacter));
}

void MainWindow::display_numberOfStartCreatingCharacter(quint16 numberOfStartCreatingCharacter)
{
    ui->numberOfStartCreatingCharacter->setText(tr("numberOfStartCreatingCharacter: %1").arg(numberOfStartCreatingCharacter));
}

void MainWindow::display_numberOfStartCreatedCharacter(quint16 numberOfStartCreatedCharacter)
{
    ui->numberOfStartCreatedCharacter->setText(tr("numberOfStartCreatedCharacter: %1").arg(numberOfStartCreatedCharacter));
}

void MainWindow::updateClientListStatus()
{
/*    ui->listClients->clear();
    for (auto i = multipleBotConnexion.apiToCatchChallengerClient.cbegin(), end = multipleBotConnexion.apiToCatchChallengerClient.cend(); i != end; ++i)
    {
        const CatchChallenger::Api_client_real * const key=i.key();
        (void)key;
        const MultipleBotConnectionAction::CatchChallengerClient * const value=i.value();
        switch(value->stat)
        {
        case MultipleBotConnectionAction::Status_None:
            ui->listClients->addItem("Status_None");
            break;
        case MultipleBotConnectionAction::Status_Connecting:
            ui->listClients->addItem("Status_Connecting");
            break;
        case MultipleBotConnectionAction::Status_Connected:
            ui->listClients->addItem("Status_Connected");
            break;
        case MultipleBotConnectionAction::Status_WaitProtocol:
            ui->listClients->addItem("Status_WaitProtocol");
            break;
        case MultipleBotConnectionAction::Status_WaitLogin:
            ui->listClients->addItem("Status_WaitLogin");
            break;
        case MultipleBotConnectionAction::Status_Logged:
            ui->listClients->addItem("Status_Logged");
            break;
        case MultipleBotConnectionAction::Status_WaitDataPack:
            ui->listClients->addItem("Status_WaitDataPack");
            break;
        case MultipleBotConnectionAction::Status_HaveDatapack:
            ui->listClients->addItem("Status_HaveDatapack");
            break;
        case MultipleBotConnectionAction::Status_CreatingCharacter:
            ui->listClients->addItem("Status_CreatingCharacter");
            break;
        case MultipleBotConnectionAction::Status_CreatedCharacter:
            ui->listClients->addItem("Status_CreatedCharacter");
            break;
        case MultipleBotConnectionAction::Status_SelectingCharacter:
            ui->listClients->addItem("Status_SelectingCharacter");
            break;
        case MultipleBotConnectionAction::Status_SelectingCharacterAfterCreation:
            ui->listClients->addItem("Status_SelectingCharacterAfterCreation");
            break;
        case MultipleBotConnectionAction::Status_SelectedCharacter:
            ui->listClients->addItem("Status_SelectedCharacter");
            break;
        case MultipleBotConnectionAction::Status_OnMap:
            ui->listClients->addItem("Status_OnMap");
            break;
        default:
            ui->listClients->addItem("???");
        }
    }*/
}

void MainWindow::on_serverList_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_serverListSelect_clicked();
}

void MainWindow::on_serverListSelect_clicked()
{
    const QList<QTreeWidgetItem *> &selectedItems=ui->serverList->selectedItems();
    if(selectedItems.size()!=1)
        return;

    const QTreeWidgetItem * const selectedItem=selectedItems.at(0);
    unsigned int serverSelectedIndex=selectedItem->data(99,99).toUInt();

    const CatchChallenger::ServerFromPoolForDisplay &serverSelected=serverOrdenedList.at(serverSelectedIndex);
    const uint8_t &charactersGroupIndex=serverSelected.charactersGroupIndex;
    if(charactersGroupIndex>=characterEntryList.size())
    {
        ui->groupBox_Server->setEnabled(false);
        QMessageBox::critical(this,tr("Error"),tr("The group index is wrong, maybe the server list have change"));
        return;
    }
    const quint32 &connectionTargetCount=ui->connexionCountTarget->value();
    if(ui->multipleConnexion->isChecked())
        if(serverSelected.maxPlayer<65533 && serverSelected.maxPlayer>=serverSelected.currentPlayer)
        {
            const quint32 &freeSlot=serverSelected.maxPlayer-serverSelected.currentPlayer;
            if(connectionTargetCount>freeSlot)
            {
                ui->groupBox_Server->setEnabled(false);
                QMessageBox::critical(this,tr("Error"),tr("Too many connexion count target for the free slot"));
                return;
            }
        }
    multipleBotConnexion.serverSelect(serverSelected.charactersGroupIndex,serverSelected.uniqueKey);

    ui->groupBox_Server->setEnabled(false);
    if(!ui->multipleConnexion->isChecked())
    {
        ui->groupBox_char->setEnabled(true);
        if(characterEntryList.at(charactersGroupIndex).empty())
        {
            if(ui->autoCreateCharacter->isChecked())
            {
                qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0 auto create";
                //auto create do into multipleBotConnexion.serverSelect()
                ui->groupBox_char->setEnabled(false);
            }
            else
            {
                ui->characterList->setStyleSheet("background-color:rgb(255,0,0);");
                ui->groupBox_char->setEnabled(false);
                ui->characterList->addItem("No character exists AND auto create disabled");
                QMessageBox::warning(this,"Error","No character exists AND auto create disabled");
            }
            return;
        }
        else
        {
            unsigned int index=0;
            while(index<characterEntryList.at(charactersGroupIndex).size())
            {
                const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).at(index);
                ui->characterList->addItem(QString::fromStdString(character.pseudo),character.character_id);
                index++;
            }
            ui->characterList->setEnabled(true);
        }
    }
    else
    {
        ui->groupBox_char->setEnabled(false);
        if(characterEntryList.size()>0)
        {
            if(characterEntryList.at(charactersGroupIndex).empty())
            {
                qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0";
                return;
            }
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(charactersGroupIndex).front();
            multipleBotConnexion.characterSelectForFirstCharacter(character.character_id);
        }
        else
        {
            qDebug() << "MainWindow::on_serverListSelect_clicked(): ui->characterList->count()==0";
        }
    }
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
}

void MainWindow::addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
{
    item->setText(0,QString::fromStdString(logicialGroup.name));
    {
        //to order the group
        std::vector<std::string> keys;
        for( const auto& n : logicialGroup.logicialGroupList )
            keys.push_back(n.first);
        std::sort(keys.begin(), keys.end());
        //list the group
        unsigned int index=0;
        while(index<keys.size())
        {
            QTreeWidgetItem * const itemGroup=new QTreeWidgetItem(item);
            CatchChallenger::LogicialGroup * logicialGroupSub=logicialGroup.logicialGroupList[keys.at(index)];
            addToServerList(*logicialGroupSub,itemGroup,currentDate,fullView);
            index++;
        }
    }
    {
        //qSort(logicialGroup.servers);-> problem to linker, fix it
        //list the server
        unsigned int index=0;
        while(index<logicialGroup.servers.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            std::string text;
            std::string groupText;
            if(serverByCharacterGroup.size()>1)
                groupText=" ("+std::to_string(serverByCharacterGroup.value(server.charactersGroupIndex).second)+")";
            std::string name=server.name;
            if(name.empty())
                name=tr("Default server").toStdString();
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    if(!server.description.empty())
                        text+=" "+tr("%1s played").arg(server.playedTime).toStdString();
                    else
                        text+="\n"+tr("%1s played").arg(server.playedTime).toStdString();
                }
                if(!server.description.empty())
                    text+="\n"+server.description;
            }
            else
            {
                if(server.description.empty())
                    text=name+groupText;
                else
                    text=name+groupText+" - "+server.description;
            }
            itemServer->setText(0,QString::fromStdString(text));

            //do the icon here
            if(server.playedTime>5*365*24*3600)
                itemServer->setToolTip(0,tr("Played time greater than 5y, bug?"));
            else if(server.lastConnect>0 && server.lastConnect<1420070400)
                itemServer->setToolTip(0,tr("Played before 2015, bug?"));
            else if(server.maxPlayer<=65533 && (server.maxPlayer<server.currentPlayer || server.maxPlayer==0))
            {
                if(server.maxPlayer<server.currentPlayer)
                    itemServer->setToolTip(0,tr("maxPlayer<currentPlayer"));
                else
                    itemServer->setToolTip(0,tr("maxPlayer==0"));
            }
            if(server.maxPlayer<=65533)
                itemServer->setText(1,QStringLiteral("%1/%2").arg(server.currentPlayer).arg(server.maxPlayer));
            itemServer->setData(99,99,server.serverOrdenedListIndex);
            index++;
        }
    }
}

void MainWindow::datapackIsReady()
{
    ui->groupBox_Server->setEnabled(true);
    if(!ui->multipleConnexion->isChecked())
    {
        ui->autoCreateCharacter->setEnabled(true);
        ui->groupBox_char->setEnabled(true);
    }
    QtDatapackClientLoader::datapackLoader->parseDatapack(QCoreApplication::applicationDirPath().toStdString()+"/datapack/");
}

void MainWindow::datapackMainSubIsReady()
{
    if(!CommonSettingsServer::commonSettingsServer.chat_allow_all && !CommonSettingsServer::commonSettingsServer.chat_allow_local)
    {
    }
    QtDatapackClientLoader::datapackLoader->parseDatapackMainSub(
                CommonSettingsServer::commonSettingsServer.mainDatapackCode,
                CommonSettingsServer::commonSettingsServer.subDatapackCode
                );
}

void MainWindow::on_autoCreateCharacter_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());
}

void MainWindow::all_player_connected()
{
    /*if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
    {
        QMessageBox::critical(this,tr("Error"),tr("Only the plant by player is supported for now!"));
        QCoreApplication::exit(555);
    }*/
    qDebug() << "MainWindow::all_player_connected()";
}

void MainWindow::all_player_on_map()
{
    qDebug() << "MainWindow::all_player_on_map()";
    botTargetList=new BotTargetList(
                multipleBotConnexion.apiToCatchChallengerClient,
                multipleBotConnexion.connectedSocketToCatchChallengerClient,
                multipleBotConnexion.sslSocketToCatchChallengerClient,
                static_cast<ActionsAction *>(multipleBotConnexion.botInterface)
                );
    SocialChat::socialChat=new SocialChat();
    //botTargetList->show();
    this->hide();
    botTargetList->hide();
    botTargetList->loadAllBotsInformation();
}

void MainWindow::on_host_returnPressed()
{
    on_connect_clicked();
}

void MainWindow::on_showPassword_toggled(bool)
{
    if(ui->showPassword->isChecked())
        ui->pass->setEchoMode(QLineEdit::Normal);
    else
        ui->pass->setEchoMode(QLineEdit::Password);
}
