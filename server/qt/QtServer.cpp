#include "QtServer.hpp"
#include "QFakeServer.hpp"
#include "QFakeSocket.hpp"
#include "timer/QtPlayerUpdater.hpp"
#include "timer/QtTimeRangeEventScanBase.hpp"
#include "db/QtDatabaseSQLite.hpp"
#include "QtClientMapManagement.hpp"
#include "../base/GlobalServerData.hpp"
#include "../base/BroadCastWithoutSender.hpp"
#include "../base/LocalClientHandlerWithoutSender.hpp"
#include "../base/ClientNetworkReadWithoutSender.hpp"
#include "../base/ClientMapManagement/MapVisibilityAlgorithm_WithoutSender.hpp"
#ifdef CATCHCHALLENGER_SOLO
#include "QFakeServer.hpp"
#endif
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QDebug>
#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
#include <QDataStream>
#include <QTcpSocket>
#endif

QtServerPrivateVariables *QtServer::qtServerPrivateVariables=nullptr;//point to defer allocation after, else connect() of Qt not work

QtServer::QtServer()
{
    if(QtServer::qtServerPrivateVariables==nullptr)
        QtServer::qtServerPrivateVariables=new QtServerPrivateVariables();
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<QSqlDatabase>("QSqlDatabase");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");
    if(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync>0)
        if(!connect(&QtServer::qtServerPrivateVariables->positionSync,&QTimer::timeout,this,&QtServer::positionSync,Qt::QueuedConnection))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
    if(!connect(&QtServer::qtServerPrivateVariables->ddosTimer,&QTimer::timeout,this,&QtServer::ddosTimer,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #ifdef CATCHCHALLENGER_SOLO
    if(!connect(&QFakeServer::server,&QFakeServer::newConnection,this,&QtServer::newConnection,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(NOTCPSOCKET) && !defined(NOSINGLEPLAYER) && defined(CATCHCHALLENGER_MULTI)
    if(!connect(&server,&QTcpServer::newConnection,this,&QtServer::newConnection,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #endif
    #endif
    if(!connect(this,&QtServer::try_stop_server,this,&QtServer::stop_internal_server))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    CatchChallenger::QtDatabaseSQLite *Qtdb_login=new CatchChallenger::QtDatabaseSQLite();
    Qtdb_login->setObjectName("db_login");
    Qtdb_login->dbThread.setObjectName("db_login");
    CatchChallenger::GlobalServerData::serverPrivateVariables.db_login=Qtdb_login;

    CatchChallenger::QtDatabaseSQLite *Qtdb_base=new CatchChallenger::QtDatabaseSQLite();
    Qtdb_base->setObjectName("db_base");
    Qtdb_base->dbThread.setObjectName("db_base");
    CatchChallenger::GlobalServerData::serverPrivateVariables.db_base=Qtdb_base;

    CatchChallenger::QtDatabaseSQLite *Qtdb_common=new CatchChallenger::QtDatabaseSQLite();
    Qtdb_common->setObjectName("db_common");
    Qtdb_common->dbThread.setObjectName("db_common");
    CatchChallenger::GlobalServerData::serverPrivateVariables.db_common=Qtdb_common;

    CatchChallenger::QtDatabaseSQLite *Qtdb_server=new CatchChallenger::QtDatabaseSQLite();
    Qtdb_server->setObjectName("db_server");
    Qtdb_server->dbThread.setObjectName("db_server");
    CatchChallenger::GlobalServerData::serverPrivateVariables.db_server=Qtdb_server;

    /*
    do via direct connection
    if(!connect(&QtServer::qtServerPrivateVariables->player_updater,&CatchChallenger::QtPlayerUpdater::newConnectedPlayer,
                &CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender,&CatchChallenger::BroadCastWithoutSender::receive_instant_player_number,
                Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&QtServer::qtServerPrivateVariables->timeRangeEventScan,&CatchChallenger::QtTimeRangeEventScan::timeRangeEventTrigger,
                &CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender,&CatchChallenger::BroadCastWithoutSender::timeRangeEventTrigger,
                Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }*/
    if(!connect(this,&QtServer::stop_internal_server_signal,this,&QtServer::stop_internal_server_slot,Qt::QueuedConnection))
        abort();

    if(QtServer::qtServerPrivateVariables->timer_to_send_insert_move_remove==nullptr)
        QtServer::qtServerPrivateVariables->timer_to_send_insert_move_remove=new QTimer();
    if(QtServer::qtServerPrivateVariables->timer_city_capture==nullptr)
        QtServer::qtServerPrivateVariables->timer_city_capture=new QTimer();
    if(!connect(QtServer::qtServerPrivateVariables->timer_to_send_insert_move_remove,	&QTimer::timeout,this,&QtServer::send_insert_move_remove,Qt::QueuedConnection))
        abort();
    if(!QtServer::qtServerPrivateVariables->timer_city_capture->isActive())
        QtServer::qtServerPrivateVariables->timer_city_capture->start(24*3600*1000);
    if(!QtServer::qtServerPrivateVariables->timer_to_send_insert_move_remove->isActive())
        QtServer::qtServerPrivateVariables->timer_to_send_insert_move_remove->start(20);

    if(!QtServer::qtServerPrivateVariables->positionSync.isActive())
        QtServer::qtServerPrivateVariables->positionSync.start(20);
    if(!QtServer::qtServerPrivateVariables->ddosTimer.isActive())
        QtServer::qtServerPrivateVariables->ddosTimer.start(20);
    if(!QtServer::qtServerPrivateVariables->player_updater.isActive())
        QtServer::qtServerPrivateVariables->player_updater.start();
    if(!QtServer::qtServerPrivateVariables->timeRangeEventScan.isActive())
        QtServer::qtServerPrivateVariables->timeRangeEventScan.start(1000);
}

QtServer::~QtServer()
{
    auto i=client_list.begin();
    while(i!=client_list.cend())
    {
        CatchChallenger::Client *client=(*i);
        client->disconnectClient();
        delete client;
        ++i;
    }
    client_list.clear();
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server!=NULL)
    {
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
        delete CatchChallenger::GlobalServerData::serverPrivateVariables.db_server;
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_server=NULL;
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common!=NULL)
    {
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
        delete CatchChallenger::GlobalServerData::serverPrivateVariables.db_common;
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_common=NULL;
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base!=NULL)
    {
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
        delete CatchChallenger::GlobalServerData::serverPrivateVariables.db_base;
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_base=NULL;
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login!=NULL)
    {
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
        delete CatchChallenger::GlobalServerData::serverPrivateVariables.db_login;
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_login=NULL;
    }
}

void QtServer::preload_the_city_capture()
{
    if(!connect(QtServer::qtServerPrivateVariables->timer_city_capture,&QTimer::timeout,this,&QtServer::load_next_city_capture,Qt::QueuedConnection))
        abort();
    if(!connect(QtServer::qtServerPrivateVariables->timer_city_capture,&QTimer::timeout,&CatchChallenger::Client::startTheCityCapture))
        abort();
    BaseServer::preload_the_city_capture();
}

void QtServer::preload_finish()
{
    BaseServer::preload_finish();

    moveToThread(QCoreApplication::instance()->thread());
    emit is_started(true);
}

void QtServer::start()
{
    need_be_started();
}

bool QtServer::isListen()
{
    return (stat==Up);
}

bool QtServer::isStopped()
{
    return (stat==Down);
}

void QtServer::stop()
{
    std::cerr << "QtServer::stop()" << std::endl;
    try_stop_server();
}

void QtServer::send_insert_move_remove()
{
    CatchChallenger::MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender.generalPurgeBuffer();
}

void QtServer::positionSync()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
}

void QtServer::ddosTimer()
{
    CatchChallenger::LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.doAllAction();
    CatchChallenger::BroadCastWithoutSender::broadCastWithoutSender.doDDOSChat();
    CatchChallenger::ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.doDDOSAction();
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void QtServer::removeOneClient()
{
    QIODevice *socket=qobject_cast<QIODevice *>(QObject::sender());
    if(socket==NULL)
    {
        qDebug() << ("removeOneClient(): NULL CatchChallenger::QtClient at disconnection");
        return;
    }
    for(CatchChallenger::Client * client : client_list)
    {
        switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
        {
            case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
            {
                QtMapVisibilityAlgorithm_Simple_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_Simple_StoreOnSender *>(client);
                if(c->socket==socket)
                {
                    delete c;
                    client_list.erase(c);
                    break;
                }
            }
            break;
            case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
            {
                QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *>(client);
                if(c->socket==socket)
                {
                    delete c;
                    client_list.erase(c);
                    break;
                }
            }
            break;
            default:
            case CatchChallenger::MapVisibilityAlgorithmSelection_None:
            {
                QtMapVisibilityAlgorithm_None *c=static_cast<QtMapVisibilityAlgorithm_None *>(client);
                if(c->socket==socket)
                {
                    delete c;
                    client_list.erase(c);
                    break;
                }
            }
            break;
        }
    }
}

/////////////////////////////////////// player related //////////////////////////////////////

void QtServer::newConnection()
{
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            qDebug() << ("newConnection(): new CatchChallenger::QtClient connected by fake socket");
            CatchChallenger::Client *client=nullptr;
            switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
                    client=new QtMapVisibilityAlgorithm_Simple_StoreOnSender(socket);
                break;
                case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
                    client=new QtMapVisibilityAlgorithm_WithBorder_StoreOnSender(socket);
                break;
                default:
                case CatchChallenger::MapVisibilityAlgorithmSelection_None:
                    client=new QtMapVisibilityAlgorithm_None(socket);
                break;
            }
            connect_the_last_client(client,socket);

            QByteArray data;
            data.resize(1);
            data[0x00]=0x00;
            socket->write(data.constData(),data.size());
        }
        else
            qDebug() << ("NULL CatchChallenger::QtClient at BaseServer::newConnection()");
    }
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(NOTCPSOCKET) && !defined(NOSINGLEPLAYER) && defined(CATCHCHALLENGER_MULTI)
    while(server.hasPendingConnections())
    {
        QTcpSocket *socket = server.nextPendingConnection();
        if(socket!=NULL)
        {
            qDebug() << ("newConnection(): new CatchChallenger::QtClient connected by TCP socket");
            CatchChallenger::Client *client=nullptr;
            switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
                    client=new QtMapVisibilityAlgorithm_Simple_StoreOnSender(socket);
                break;
                case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
                    client=new QtMapVisibilityAlgorithm_WithBorder_StoreOnSender(socket);
                break;
                default:
                case CatchChallenger::MapVisibilityAlgorithmSelection_None:
                    client=new QtMapVisibilityAlgorithm_None(socket);
                break;
            }
            connect_the_last_client(client,socket);

            QByteArray data;
            data.resize(1);
            data[0x00]=0x00;
            socket->write(data.constData(),data.size());
        }
        else
            qDebug() << ("NULL CatchChallenger::QtClient at BaseServer::newConnection() TCP socket");
    }
    #endif
    #endif
}

void QtServer::connect_the_last_client(CatchChallenger::Client *client, QIODevice *socket)
{
    switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
        {
            QtMapVisibilityAlgorithm_Simple_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_Simple_StoreOnSender *>(client);
            if(!connect(socket,&QIODevice::readyRead,c,&QtMapVisibilityAlgorithm_Simple_StoreOnSender::parseIncommingData,Qt::QueuedConnection))
                abort();
        }
        break;
        case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
        {
            QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *>(client);
            if(!connect(socket,&QIODevice::readyRead,c,&QtMapVisibilityAlgorithm_WithBorder_StoreOnSender::parseIncommingData,Qt::QueuedConnection))
                abort();
        }
        break;
        default:
        case CatchChallenger::MapVisibilityAlgorithmSelection_None:
        {
            QtMapVisibilityAlgorithm_None *c=static_cast<QtMapVisibilityAlgorithm_None *>(client);
            if(!connect(socket,&QIODevice::readyRead,c,&QtMapVisibilityAlgorithm_None::parseIncommingData,Qt::QueuedConnection))
                abort();
        }
        break;
    }
    if(!connect(socket,&QObject::destroyed,this,&QtServer::removeOneClient,Qt::DirectConnection))
        abort();
    client_list.insert(client);
}

void QtServer::load_next_city_capture()
{
    BaseServer::load_next_city_capture();
}

bool QtServer::check_if_now_stopped()
{
    std::cerr << "QtServer::check_if_now_stopped()" << std::endl;
    if(client_list.size()!=0)
        return false;
    if(stat!=InDown)
        return false;

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server!=NULL && CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->isConnected())
        if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->databaseType()!=CatchChallenger::DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->databaseType()) << std::endl;
            abort();
        }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common!=NULL && CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->isConnected())
        if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->databaseType()!=CatchChallenger::DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->databaseType()) << std::endl;
            abort();
        }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login!=NULL && CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->isConnected())
        if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->databaseType()!=CatchChallenger::DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->databaseType()) << std::endl;
            abort();
        }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base!=NULL && CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->isConnected())
        if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->databaseType()!=CatchChallenger::DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->databaseType()) << std::endl;
            abort();
        }
    #endif
    #endif
    qDebug() << ("Fully stopped");
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->databaseType())))
                                 .arg(QString::fromStdString(CatchChallenger::GlobalServerData::serverSettings.database_login.host)));
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->databaseType())))
                                 .arg(QString::fromStdString(CatchChallenger::GlobalServerData::serverSettings.database_base.host)));
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->databaseType())))
                                 .arg(QString::fromStdString(CatchChallenger::GlobalServerData::serverSettings.database_common.host)));
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    }
    if(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->isConnected())
    {
        qDebug() << (QStringLiteral("Disconnected to %1 at %2")
                                 .arg(QString::fromStdString(CatchChallenger::DatabaseBase::databaseTypeToString(CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->databaseType())))
                                 .arg(QString::fromStdString(CatchChallenger::GlobalServerData::serverSettings.database_server.host)));
        CatchChallenger::GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
    }
    stat=Down;
    is_started(false);

    unload_the_data();
    return true;
}

//call by normal stop
void QtServer::stop_internal_server()
{
    std::cerr << "QtServer::stop_internal_server()" << std::endl;
    moveToThread(QThread::currentThread());
    std::cerr << "QtServer::stop_internal_server() moveToThread(QThread::currentThread())" << std::endl;
    if(stat!=Up && stat!=InDown)
    {
        std::cerr << "QtServer::stop_internal_server() stat!=Up && stat!=InDown: " << stat << std::endl;
        if(stat!=Down)
            qDebug() << ("Is in wrong stat for stopping: "+QString::number((int)stat));
        if(stat!=InDown)
        {
            std::cerr << "QtServer::stop_internal_server() emit stop_internal_server_signal(): " << isRunning() << std::endl;
            if(isRunning())
                emit stop_internal_server_signal();
            else if(stat==Down)
            {
                stop_internal_server_slot();
                is_started(false);
            }
            else
                stop_internal_server_slot();
        }
        return;
    }
    qDebug() << ("Try stop");
    stat=InDown;
    std::cerr << "QtServer::stop_internal_server() emit stop_internal_server_signal()" << std::endl;
    emit stop_internal_server_signal();
}

void QtServer::stop_internal_server_slot()
{
    std::cerr << "QtServer::stop_internal_server_slot()" << std::endl;
    //#ifndef CATCHCHALLENGER_SOLO
    auto i=client_list.begin();
    while(i!=client_list.cend())
    {
        switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
        {
            case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
            {
                QtMapVisibilityAlgorithm_Simple_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_Simple_StoreOnSender *>(*i);
                (*i)->disconnectClient();
                delete c;
                break;
            }
            break;
            case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
            {
                QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *c=static_cast<QtMapVisibilityAlgorithm_WithBorder_StoreOnSender *>(*i);
                (*i)->disconnectClient();
                delete c;
                break;
            }
            break;
            default:
            case CatchChallenger::MapVisibilityAlgorithmSelection_None:
            {
                QtMapVisibilityAlgorithm_None *c=static_cast<QtMapVisibilityAlgorithm_None *>(*i);
                (*i)->disconnectClient();
                delete c;
                break;
            }
            break;
        }
        ++i;
    }
    //#endif
    client_list.clear();
    #ifdef CATCHCHALLENGER_SOLO
    QFakeServer::server.disconnectedSocket();
    QFakeServer::server.close();
    #endif

    check_if_now_stopped();
}

void QtServer::quitForCriticalDatabaseQueryFailed()
{
    emit haveQuitForCriticalDatabaseQueryFailed();
    stop_internal_server();
}

void QtServer::loadAndFixSettings()
{
    if(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync==0)
    {
        QtServer::qtServerPrivateVariables->positionSync.stop();
    }
    BaseServer::loadAndFixSettings();
}

void QtServer::setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start)
{
}

void QtServer::preload_the_visibility_algorithm()
{
}

void QtServer::unload_the_visibility_algorithm()
{
}

void QtServer::unload_the_events()
{
}

#if defined(CATCHCHALLENGER_SOLO) && ! defined(NOTCPSOCKET) && !defined(NOSINGLEPLAYER) && defined(CATCHCHALLENGER_MULTI)
void QtServer::openToLan(QString name, bool allowInternet)
{
    server.listen();
    //broadcastLan.bind(QHostAddress::Any, 42489);
    if(!connect(&broadcastLanTimer,&QTimer::timeout,this,&QtServer::sendBroadcastServer))
        abort();
    QDataStream stream(&dataToSend,QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_4);
    stream << name;
    stream << server.serverPort();
    if(dataToSend.isEmpty())
        abort();
    emit emitLanPort(server.serverPort());
    broadcastLanTimer.start(500);

    CatchChallenger::GameServerSettings formatedServerSettings=getSettings();
    formatedServerSettings.max_players=99;//> 1 to allow open to lan
    //formatedServerSettings.sendPlayerNumber = true;//> not work because connected_players is blocked to 1
    formatedServerSettings.everyBodyIsRoot                                      = false;
    setSettings(formatedServerSettings);
}

bool QtServer::openIsOpenToLan()
{
    return server.isListening();
}

void QtServer::sendBroadcastServer()
{
    broadcastLan.writeDatagram(dataToSend,QHostAddress::Broadcast,42490);
}
#endif
