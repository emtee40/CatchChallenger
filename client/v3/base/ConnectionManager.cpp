// Copyright 2021 CatchChallenger
#include "ConnectionManager.hpp"

#include <QObject>
#include <QStandardPaths>
#include <iostream>
#include <vector>

#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../libqtcatchchallenger/Api_client_real.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libqtcatchchallenger/Settings.hpp"

using std::placeholders::_1;

ConnectionManager *ConnectionManager::instance_ = nullptr;

ConnectionManager::ConnectionManager() {
  client = nullptr;
  socket = nullptr;
#ifndef NOTCPSOCKET
  realSslSocket = nullptr;
#endif
#ifndef NOWEBSOCKET
  realWebSocket = nullptr;
#endif
#ifdef CATCHCHALLENGER_SOLO
  fakeSocket = nullptr;
#endif
  this->datapckFileSize = 0;

  haveDatapack = false;
  haveDatapackMainSub = false;
  datapackIsParsed = false;
}

ConnectionManager *ConnectionManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new (std::nothrow) ConnectionManager();
  }
  return instance_;
}

void ConnectionManager::ClearInstance() {
  /*
  if (instance_) {
    delete instance_;
    instance_ = nullptr;
  }*/
}

void ConnectionManager::connectToServer(ConnectionInfo connexionInfo,
                                        QString login, QString pass) {
  haveDatapack = false;
  haveDatapackMainSub = false;
  datapackIsParsed = false;

  if (socket != NULL) {
    socket->disconnectFromHost();
    socket->abort();
    delete socket;
    socket = NULL;
#ifndef NOTCPSOCKET
    realSslSocket = nullptr;
#endif
#ifndef NOWEBSOCKET
    realWebSocket = nullptr;
#endif
  }
#if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
  #error can t be both NOTCPSOCKET and NOWEBSOCKET disabled
#else
  #ifndef NOTCPSOCKET
  if (!connexionInfo.host.isEmpty()) {
    realSslSocket = new QSslSocket();
    socket = new CatchChallenger::ConnectedSocket(realSslSocket);
  } else {
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket = new QFakeSocket();
    socket = new CatchChallenger::ConnectedSocket(fakeSocket);
    #else
    std::cerr << "host is empty" << std::endl;
    abort();
    #endif
  }
  #endif
  #ifndef NOWEBSOCKET
  if (socket == nullptr) {
    if (!connexionInfo.ws.isEmpty()) {
      realWebSocket = new QWebSocket();
      socket = new CatchChallenger::ConnectedSocket(realWebSocket);
    } else {
      std::cerr << "ws is empty" << std::endl;
      abort();
    }
  }
  #endif
#endif
  // work around for the resetAll() of protocol
  const std::string mainDatapackCode =
      CommonSettingsServer::commonSettingsServer.mainDatapackCode;
  const std::string subDatapackCode =
      CommonSettingsServer::commonSettingsServer.subDatapackCode;
  CatchChallenger::Api_client_real *client =
      new CatchChallenger::Api_client_real(socket);
  CommonSettingsServer::commonSettingsServer.mainDatapackCode =
      mainDatapackCode;
  CommonSettingsServer::commonSettingsServer.subDatapackCode = subDatapackCode;
  /*    if(serverMode==ServerMode_Internal)
          client->tryLogin("admin",pass.toStdString());
      else*/
  client->tryLogin(login.toStdString(), pass.toStdString());

  if (!connect(client, &CatchChallenger::Api_protocol_Qt::QtnewError, this,
               &ConnectionManager::newError))
    abort();
  if (!connect(client, &CatchChallenger::Api_protocol_Qt::Qtmessage, this,
               &ConnectionManager::message))
    abort();

  if (!connect(client, &CatchChallenger::Api_client_real::Qtdisconnected, this,
               &ConnectionManager::disconnected))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtnotLogged, this,
               &ConnectionManager::disconnected))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QthaveTheDatapack,
               this, &ConnectionManager::haveTheDatapack))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QthaveTheDatapackMainSub,
               this, &ConnectionManager::haveTheDatapackMainSub))
    abort();

  if (!connect(client, &CatchChallenger::Api_client_real::Qtlogged, this,
               &ConnectionManager::Qtlogged, Qt::QueuedConnection))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtdatapackSizeBase,
               this, &ConnectionManager::QtdatapackSizeBase))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtdatapackSizeMain,
               this, &ConnectionManager::QtdatapackSizeMain))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtdatapackSizeSub,
               this, &ConnectionManager::QtdatapackSizeSub))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::progressingDatapackFileBase,
               this, &ConnectionManager::progressingDatapackFileBase))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::progressingDatapackFileMain,
               this, &ConnectionManager::progressingDatapackFileMain))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::progressingDatapackFileSub,
               this, &ConnectionManager::progressingDatapackFileSub))
    abort();

  if (!connect(client, &CatchChallenger::Api_client_real::Qtprotocol_is_good,
               this, &ConnectionManager::protocol_is_good))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtconnectedOnLoginServer,
               this, &ConnectionManager::connectedOnLoginServer))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtconnectingOnGameServer,
               this, &ConnectionManager::connectingOnGameServer))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtconnectedOnGameServer, this,
               &ConnectionManager::connectedOnGameServer))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QthaveDatapackMainSubCode,
               this, &ConnectionManager::haveDatapackMainSubCode))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtgatewayCacheUpdate,
               this, &ConnectionManager::gatewayCacheUpdate))
    abort();

  if (!connect(client, &CatchChallenger::Api_client_real::QthaveCharacter, this,
               &ConnectionManager::QthaveCharacter, Qt::QueuedConnection))
    abort();

  if (!connexionInfo.proxyHost.isEmpty()) {
    QNetworkProxy proxy;
#ifndef NOTCPSOCKET
    if (realSslSocket != nullptr) proxy = realSslSocket->proxy();
#endif
#ifndef NOWEBSOCKET
    if (realWebSocket != nullptr) proxy = realWebSocket->proxy();
#endif
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName(connexionInfo.proxyHost);
    proxy.setPort(connexionInfo.proxyPort);
#ifndef NOTCPSOCKET
    if (realSslSocket != nullptr) realSslSocket->setProxy(proxy);
#endif
#ifndef NOWEBSOCKET
    if (realWebSocket != nullptr) realWebSocket->setProxy(proxy);
#endif
  }
  connexionInfo.connexionCounter++;
  connexionInfo.lastConnexion =
      static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);
  this->client = static_cast<CatchChallenger::Api_protocol_Qt *>(client);
  connectTheExternalSocket(connexionInfo, client);

  // connect the datapack loader
  if (!connect(QtDatapackClientLoader::datapackLoader,
               &QtDatapackClientLoader::datapackParsed, this,
               &ConnectionManager::datapackParsed, Qt::QueuedConnection))
    abort();
  if (!connect(QtDatapackClientLoader::datapackLoader,
               &QtDatapackClientLoader::datapackParsedMainSub, this,
               &ConnectionManager::datapackParsedMainSub, Qt::QueuedConnection))
    abort();
  if (!connect(QtDatapackClientLoader::datapackLoader,
               &QtDatapackClientLoader::datapackChecksumError, this,
               &ConnectionManager::datapackChecksumError, Qt::QueuedConnection))
    abort();

#ifndef NOTCPSOCKET
  if (realSslSocket != nullptr) {
    if (!connect(
            realSslSocket,
            static_cast<void (QSslSocket::*)(const QList<QSslError> &errors)>(
                &QSslSocket::sslErrors),
            this, &ConnectionManager::sslErrors, Qt::QueuedConnection))
      abort();
    if (!connect(realSslSocket, &QSslSocket::stateChanged, this,
                 &ConnectionManager::stateChanged, Qt::QueuedConnection))
      abort();
    if (!connect(
            realSslSocket,
            static_cast<void (QSslSocket::*)(QAbstractSocket::SocketError)>(
                &QSslSocket::error),
            this, &ConnectionManager::error, Qt::QueuedConnection))
      abort();

    socket->connectToHost(connexionInfo.host, connexionInfo.port);
  }
#endif
#ifndef NOWEBSOCKET
  if (realWebSocket != nullptr) {
    if (!connect(realWebSocket, &QWebSocket::stateChanged, this,
                 &ConnectionManager::stateChanged, Qt::DirectConnection))
      abort();
    if (!connect(
            realWebSocket,
            static_cast<void (QWebSocket::*)(QAbstractSocket::SocketError)>(
                &QWebSocket::error),
            this, &ConnectionManager::error, Qt::QueuedConnection))
      abort();

    QUrl url{QString(connexionInfo.ws)};
    QNetworkRequest request{url};
    request.setRawHeader("Sec-WebSocket-Protocol", "binary");
    realWebSocket->open(request);
  }
#endif
#ifdef CATCHCHALLENGER_SOLO
  if (fakeSocket != nullptr) {
    if (!connect(fakeSocket, &QFakeSocket::stateChanged, this,
                 &ConnectionManager::stateChanged, Qt::DirectConnection))
      abort();
    if (!connect(
            fakeSocket,
            static_cast<void (QFakeSocket::*)(QAbstractSocket::SocketError)>(
                &QFakeSocket::error),
            this, &ConnectionManager::error, Qt::QueuedConnection))
      abort();

    fakeSocket->connectToHost();
  }
#endif
}

void ConnectionManager::selectCharacter(const uint32_t indexSubServer,
                                        const uint32_t indexCharacter) {
  if (!client->selectCharacter(indexSubServer, indexCharacter))
    errorString(
        "BaseWindow::on_serverListSelect_clicked(), wrong serverSelected "
        "internal data");
}

void ConnectionManager::disconnected(std::string reason) {
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] " << reason
            << std::endl;
  // QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the
  // reason: %1").arg(QString::fromStdString(reason)));
  if (on_error_) on_error_(reason);
  /*if(serverConnexion.contains(selectedServer))
      lastServerIsKick[serverConnexion.value(selectedServer)->host]=true;*/
}

void ConnectionManager::newError(const std::string &error,
                                 const std::string &detailedError) {
  if (on_error_) on_error_(error + "\n" + detailedError);
  if (socket != nullptr) socket->disconnectFromHost();
  if (client != nullptr) {
    client->disconnectClient();
    client->resetAll();
  }
}

void ConnectionManager::message(const std::string &message) {
  std::cerr << message << std::endl;
}

#ifndef __EMSCRIPTEN__
void ConnectionManager::sslErrors(const QList<QSslError> &errors) {
  QStringList sslErrors;
  int index = 0;
  while (index < errors.size()) {
    qDebug() << "Ssl error:" << errors.at(index).errorString();
    sslErrors << errors.at(index).errorString();
    index++;
  }
  /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
  realSocket->disconnectFromHost();*/
}
#endif

void ConnectionManager::connectTheExternalSocket(
    ConnectionInfo connexionInfo, CatchChallenger::Api_client_real *client) {
  // continue the normal procedure
  if (!connexionInfo.proxyHost.isEmpty()) {
    QNetworkProxy proxy;
#ifndef NOTCPSOCKET
    if (realSslSocket != nullptr) proxy = realSslSocket->proxy();
#endif
#ifndef NOWEBSOCKET
    if (realWebSocket != nullptr) proxy = realWebSocket->proxy();
#endif
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName(connexionInfo.proxyHost);
    proxy.setPort(connexionInfo.proxyPort);
    client->setProxy(proxy);
  }

  /*baseWindow->connectAllSignals();
  baseWindow->setMultiPlayer(true,client);*/
  QDir datapack(serverToDatapachPath(connexionInfo));
  if (!datapack.exists()) {
    if (!datapack.mkpath(datapack.absolutePath())) {
      disconnected(QObject::tr("Not able to create the folder %1")
                       .arg(datapack.absolutePath())
                       .toStdString());
      return;
    }
  }
  // work around for the resetAll() of protocol
  const std::string mainDatapackCode =
      CommonSettingsServer::commonSettingsServer.mainDatapackCode;
  const std::string subDatapackCode =
      CommonSettingsServer::commonSettingsServer.subDatapackCode;
  // if(!connexionInfo.unique_code.isEmpty())
  client->setDatapackPath(datapack.absolutePath().toStdString());
  CommonSettingsServer::commonSettingsServer.mainDatapackCode =
      mainDatapackCode;
  CommonSettingsServer::commonSettingsServer.subDatapackCode = subDatapackCode;
  // baseWindow->stateChanged(QAbstractSocket::ConnectedState);
}

QString ConnectionManager::serverToDatapachPath(
    ConnectionInfo connexionInfo) const {
  QDir datapack;
  if (connexionInfo.isCustom)
    datapack = QDir(
        QStringLiteral("%1/datapack/Custom-%2/")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(connexionInfo.unique_code));
  else
    datapack = QDir(
        QStringLiteral("%1/datapack/Xml-%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(connexionInfo.unique_code));
  return datapack.absolutePath();
}

/*QString ConnectionManager::serverToDatapachPath(ListEntryEnvolued *
selectedServer) const
{
    QDir datapack;
    if(customServerConnexion.contains(selectedServer))
    {
        if(!serverConnexion.value(selectedServer)->name.isEmpty())
             datapack=QDir(QStringLiteral("%1/datapack/%2/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->name));
        else
             datapack=QDir(QStringLiteral("%1/datapack/%2-%3/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->host).arg(serverConnexion.value(selectedServer)->port));
    }
    else
        datapack=QDir(QStringLiteral("%1/datapack/Xml-%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->unique_code));
    return datapack.absolutePath();
}*/

void ConnectionManager::stateChanged(QAbstractSocket::SocketState socketState) {
  std::cout << "ConnectionManager::stateChanged("
            << std::to_string((int)socketState) << ")" << std::endl;
  if (socketState == QAbstractSocket::ConnectedState) {
    // If uncomment: Internal problem: Api_protocol::sendProtocol()
    // !haveFirstHeader
    /*if((1
        #if !defined(NOTCPSOCKET)
            && realSslSocket==NULL
        #endif
    #if !defined(NOWEBSOCKET)
         && realWebSocket==NULL
    #endif
        )
    #ifdef CATCHCHALLENGER_SOLO
         && fakeSocket!=NULL
    #endif
            )
        if(client!=nullptr)
            client->sendProtocol();*/
    /*else
        qDebug() << "Tcp/Web socket found, skip sendProtocol(), previouslusy
       send by void Api_protocol::connectTheExternalSocketInternal()";*/
  }
  if (socketState == QAbstractSocket::UnconnectedState) {
    std::cout << "ConnectionManager::stateChanged("
              << std::to_string((int)socketState) << ") mostly quit "
              << client->stage() << std::endl;
    if (client->stage() ==
            CatchChallenger::Api_protocol::StageConnexion::Stage2 ||
        client->stage() ==
            CatchChallenger::Api_protocol::StageConnexion::Stage3)
      return;
#ifndef NOTCPSOCKET
    if (realSslSocket != NULL) {
      realSslSocket->deleteLater();
      realSslSocket = NULL;
    }
#endif
#ifndef NOWEBSOCKET
    if (realWebSocket != NULL) {
      realWebSocket->deleteLater();
      realWebSocket = NULL;
    }
#endif
#ifdef CATCHCHALLENGER_SOLO
    if (fakeSocket != NULL) {
      fakeSocket->deleteLater();
      fakeSocket = NULL;
    }
#endif
    if (socket != NULL) {
      socket->deleteLater();
      socket = NULL;
    }
    if (on_disconnect_) on_disconnect_();
  }
}

QAbstractSocket::SocketState ConnectionManager::state() {
  if (socket == nullptr) return QAbstractSocket::UnconnectedState;
  return socket->state();
}

void ConnectionManager::error(QAbstractSocket::SocketError socketError) {
  if (client != NULL)
    if (client->stage() ==
        CatchChallenger::Api_client_real::StageConnexion::Stage2)
      return;
  QString additionalText;
  if (!lastServer.isEmpty())
    additionalText = QObject::tr(" on %1").arg(lastServer);
  // resetAll();
  switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
#ifndef NOTCPSOCKET
      if (realSslSocket != NULL) return;
#endif
#ifndef NOWEBSOCKET
      if (realWebSocket != NULL) return;
#endif
      /*if(haveShowDisconnectionReason)
      {
          haveShowDisconnectionReason=false;
          return;
      }*/
      /*QMessageBox::information(this,tr("Connection closed"),tr("Connection
  closed by the server")+additionalText); break; case
  QAbstractSocket::ConnectionRefusedError:
      QMessageBox::information(this,tr("Connection closed"),tr("Connection
  refused by the server")+additionalText); break; case
  QAbstractSocket::SocketTimeoutError:
      QMessageBox::information(this,tr("Connection closed"),tr("Socket time out,
  server too long")+additionalText); break; case
  QAbstractSocket::HostNotFoundError:
      QMessageBox::information(this,tr("Connection closed"),tr("The host address
  was not found")+additionalText); break; case
  QAbstractSocket::SocketAccessError:
      QMessageBox::information(this,tr("Connection closed"),tr("The socket
  operation failed because the application lacked the required
  privileges")+additionalText); break; case
  QAbstractSocket::SocketResourceError:
      QMessageBox::information(this,tr("Connection closed"),tr("The local system
  ran out of resources")+additionalText); break; case
  QAbstractSocket::NetworkError: QMessageBox::information(this,tr("Connection
  closed"),tr("An error occurred with the network (Connection refused on game
  server?)")+additionalText); break; case
  QAbstractSocket::UnsupportedSocketOperationError:
      QMessageBox::information(this,tr("Connection closed"),tr("The requested
  socket operation is not supported by the local operating system (e.g., lack of
  IPv6 support)")+additionalText); break; case
  QAbstractSocket::SslHandshakeFailedError:
      QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS
  handshake failed, so the connection was closed")+additionalText); break;
  default:
      QMessageBox::information(this,tr("Connection error"),tr("Connection error:
  %1").arg(socketError)+additionalText);*/
      errorString(
          (QObject::tr("Connection closed by the server") + additionalText)
              .toStdString());
      break;
    case QAbstractSocket::ConnectionRefusedError:
      errorString(
          (QObject::tr("Connection refused by the server") + additionalText)
              .toStdString());
      break;
    case QAbstractSocket::SocketTimeoutError:
      errorString(
          (QObject::tr("Socket time out, server too long") + additionalText)
              .toStdString());
      break;
    case QAbstractSocket::HostNotFoundError:
      errorString(
          (QObject::tr("The host address was not found") + additionalText)
              .toStdString());
      break;
    case QAbstractSocket::SocketAccessError:
      errorString(
          (QObject::tr("The socket operation failed because the application "
                       "lacked the required privileges") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::SocketResourceError:
      errorString((QObject::tr("The local system ran out of resources") +
                   additionalText)
                      .toStdString());
      break;
    case QAbstractSocket::NetworkError:
      errorString(
          (QObject::tr("An error occurred with the network (Connection refused "
                       "on game server?)") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::UnsupportedSocketOperationError:
      errorString(
          (QObject::tr("The requested socket operation is not supported by the "
                       "local operating system (e.g., lack of IPv6 support)") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::SslHandshakeFailedError:
      errorString(
          (QObject::tr(
               "The SSL/TLS handshake failed, so the connection was closed") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::ProxyConnectionRefusedError:
      errorString((tr("Could not contact the proxy server because the "
                      "connection to that server was denied") +
                   additionalText)
                      .toStdString());
      break;
    case QAbstractSocket::ProxyConnectionClosedError:
      errorString(
          (tr("The connection to the proxy server was closed unexpectedly "
              "(before the connection to the final peer was established)") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::ProxyConnectionTimeoutError:
      errorString(
          (tr("The connection to the proxy server timed out or the proxy "
              "server stopped responding in the authentication phase") +
           additionalText)
              .toStdString());
      break;
    case QAbstractSocket::ProxyNotFoundError:
      errorString((tr("The proxy address set with setProxy() (or the "
                      "application proxy) was not found") +
                   additionalText)
                      .toStdString());
      break;
    case QAbstractSocket::ProxyProtocolError:
      errorString((tr("The connection negotiation with the proxy server "
                      "failed, because the response from the proxy server "
                      "could not be understood") +
                   additionalText)
                      .toStdString());
      break;
    default:
      errorString((QObject::tr("Connection error: %1").arg(socketError) +
                   additionalText)
                      .toStdString());
  }
}

void ConnectionManager::QtdatapackSizeBase(const uint32_t &datapckFileNumber,
                                           const uint32_t &datapckFileSize) {
  (void)datapckFileNumber;
  this->datapckFileSize = datapckFileSize;
  NotifyProgress(0, datapckFileSize);
}

void ConnectionManager::QtdatapackSizeMain(const uint32_t &datapckFileNumber,
                                           const uint32_t &datapckFileSize) {
  (void)datapckFileNumber;
  this->datapckFileSize = datapckFileSize;
  NotifyProgress(0, datapckFileSize);
}

void ConnectionManager::QtdatapackSizeSub(const uint32_t &datapckFileNumber,
                                          const uint32_t &datapckFileSize) {
  (void)datapckFileNumber;
  this->datapckFileSize = datapckFileSize;
  NotifyProgress(0, datapckFileSize);
}

void ConnectionManager::progressingDatapackFileBase(const uint32_t &size) {
  NotifyProgress(size, datapckFileSize);
}

void ConnectionManager::progressingDatapackFileMain(const uint32_t &size) {
  NotifyProgress(size, datapckFileSize);
}

void ConnectionManager::progressingDatapackFileSub(const uint32_t &size) {
  NotifyProgress(size, datapckFileSize);
}

void ConnectionManager::protocol_is_good() { NotifyStatus("Protocol is good"); }

void ConnectionManager::connectedOnLoginServer() {
  NotifyStatus("Connected on login server");
}

void ConnectionManager::connectingOnGameServer() {
  NotifyStatus("Connecting on game server");
}

void ConnectionManager::connectedOnGameServer() {
  NotifyStatus("Connected on game server");
}

void ConnectionManager::gatewayCacheUpdate(const uint8_t gateway,
                                           const uint8_t progression) {
  NotifyStatus("Connected on game server");
  NotifyProgress(progression, 100 + 1);
}

void ConnectionManager::sendDatapackContentMainSub() {
  if (client == nullptr) {
    std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
    abort();
  }
  QByteArray dataMain;
  QByteArray dataSub;
  if (Settings::settings->contains(
          "DatapackHashMain-" +
          QString::fromStdString(client->datapackPathMain())))
    dataMain = QByteArray::fromHex(
        Settings::settings
            ->value("DatapackHashMain-" +
                    QString::fromStdString(client->datapackPathMain()))
            .toString()
            .toUtf8());
  if (Settings::settings->contains(
          "DatapackHashSub-" +
          QString::fromStdString(client->datapackPathSub())))
    dataSub = QByteArray::fromHex(
        Settings::settings
            ->value("DatapackHashSub-" +
                    QString::fromStdString(client->datapackPathSub()))
            .toString()
            .toUtf8());
  client->sendDatapackContentMainSub(
      std::string(dataMain.constData(), dataMain.size()),
      std::string(dataSub.constData(), dataSub.size()));
}

void ConnectionManager::haveTheDatapack() {
  if (client == NULL) return;
#ifdef DEBUG_BASEWINDOWS
  qDebug() << "BaseWindow::haveTheDatapack()";
#endif
  if (haveDatapack) return;
  haveDatapack = true;
  Settings::settings->setValue(
      "DatapackHashBase-" + QString::fromStdString(client->datapackPathBase()),
      QString(
          QByteArray(CommonSettingsCommon::commonSettingsCommon.datapackHashBase
                         .data(),
                     static_cast<int>(CommonSettingsCommon::commonSettingsCommon
                                          .datapackHashBase.size()))
              .toHex()));

  if (client != NULL) {
    QtDatapackClientLoader::datapackLoader->parseDatapack(
        client->datapackPathBase());
  }
}

void ConnectionManager::haveTheDatapackMainSub() {
#ifdef DEBUG_BASEWINDOWS
  qDebug() << "BaseWindow::haveTheDatapackMainSub()";
#endif
  if (haveDatapackMainSub) return;
  haveDatapackMainSub = true;
  Settings::settings->setValue(
      "DatapackHashMain-" + QString::fromStdString(client->datapackPathMain()),
      QString(
          QByteArray(CommonSettingsServer::commonSettingsServer
                         .datapackHashServerMain.data(),
                     static_cast<int>(CommonSettingsServer::commonSettingsServer
                                          .datapackHashServerMain.size()))
              .toHex()));
  Settings::settings->setValue(
      "DatapackHashSub-" + QString::fromStdString(client->datapackPathSub()),
      QString(
          QByteArray(CommonSettingsServer::commonSettingsServer
                         .datapackHashServerSub.data(),
                     static_cast<int>(CommonSettingsServer::commonSettingsServer
                                          .datapackHashServerSub.size()))
              .toHex()));

  if (client != NULL) {
    QtDatapackClientLoader::datapackLoader->parseDatapackMainSub(
        client->mainDatapackCode(), client->subDatapackCode());
  }
}

void ConnectionManager::datapackParsed() {
#ifdef DEBUG_BASEWINDOWS
  qDebug() << "BaseWindow::datapackParsed()";
#endif
  datapackIsParsed = true;
  // updateConnectingStatus();
  // loadSettingsWithDatapack();
  /*{
      if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png")).exists())
          ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image:
  url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png');padding:6px
  6px 6px 14px;}")); else
          ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image:
  url(:/CC/images/interface/fight/labelBottom.png);padding:6px 6px 6px
  14px;}"));
      if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png")).exists())
          ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image:
  url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png');padding:6px
  14px 6px 6px;}")); else
          ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image:
  url(:/CC/images/interface/fight/labelTop.png);padding:6px 14px 6px 6px;}"));
  }*/
  // updatePlayerImage();
  if (on_logged_) {
    on_logged_(characterEntryList);
  }
}

void ConnectionManager::datapackParsedMainSub() {
  if (client == NULL) return;
  if (!client->setMapNumber(
          QtDatapackClientLoader::datapackLoader->get_mapToId().size()))
    newError(QObject::tr("Internal error").toStdString(),
             "No map loaded to call client->setMapNumber()");
  client->have_main_and_sub_datapack_loaded();
}

void ConnectionManager::datapackChecksumError() {
#ifdef DEBUG_BASEWINDOWS
  qDebug() << "BaseWindow::datapackChecksumError()";
#endif
  datapackIsParsed = false;
  // reset all the cached hash
  Settings::settings->remove(
      "DatapackHashBase-" + QString::fromStdString(client->datapackPathBase()));
  Settings::settings->remove(
      "DatapackHashMain-" + QString::fromStdString(client->datapackPathMain()));
  Settings::settings->remove("DatapackHashSub-" +
                             QString::fromStdString(client->datapackPathSub()));
  newError(QObject::tr("Datapack on mirror is corrupted").toStdString(),
           "The checksum sended by the server is not the same than have "
           "on the mirror");
}

void ConnectionManager::Qtlogged(
    const std::vector<std::vector<CatchChallenger::CharacterEntry>>
        &characterEntryList) {
  if (Settings::settings->contains(
          "DatapackHashBase-" +
          QString::fromStdString(client->datapackPathBase()))) {
    const QByteArray &data = QByteArray::fromHex(
        Settings::settings
            ->value("DatapackHashBase-" +
                    QString::fromStdString(client->datapackPathBase()))
            .toString()
            .toUtf8());
    client->sendDatapackContentBase(std::string(data.constData(), data.size()));
  } else {
    client->sendDatapackContentBase();
  }
  this->characterEntryList = characterEntryList;
}

void ConnectionManager::QthaveCharacter() {
  /*    if(client==nullptr)
      {
          std::cerr << "sendDatapackContentMainSub(): client==nullptr" <<
     std::endl; return;
      }
      sendDatapackContentMainSub();*/
  if (on_go_to_map_) on_go_to_map_();
  std::cout << "have character" << std::endl;
}

void ConnectionManager::haveDatapackMainSubCode() {
  /// \warn do above
  if (client == nullptr) {
    std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
    return;
  }
  sendDatapackContentMainSub();
  // updateConnectingStatus();
}

void ConnectionManager::SetOnLogged(
    std::function<
        void(std::vector<std::vector<CatchChallenger::CharacterEntry>>)>
        callback) {
  on_logged_ = callback;
}

void ConnectionManager::SetOnGoToMap(std::function<void()> callback) {
  on_go_to_map_ = callback;
}

void ConnectionManager::SetOnError(std::function<void(std::string)> callback) {
  on_error_ = callback;
}

void ConnectionManager::SetOnDisconnect(std::function<void()> callback) {
  on_disconnect_ = callback;
}

void ConnectionManager::errorString(std::string error) { on_error_(error); }