#include "ScreenTransition.hpp"
#ifdef CATCHCHALLENGER_SOLO
#include "../../server/qt/InternalServer.hpp"
#include "../libqtcatchchallenger/Settings.hpp"
#include <QStandardPaths>
#endif
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "background/CCMap.hpp"
#include "foreground/CharacterList.hpp"
#include "foreground/LoadingScreen.hpp"
#include "foreground/MainScreen.hpp"
#include "foreground/Multi.hpp"
#include "foreground/SubServer.hpp"
#include "foreground/OverMapLogic.hpp"
#include "above/OptionsDialog.hpp"
#include "above/DebugDialog.hpp"
#include "ConnexionManager.hpp"
#include "../../general/base/Version.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CompressionProtocol.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#endif
#include <iostream>
#include <QGLWidget>
#include <QComboBox>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>

void keep_screen_on(bool on) {
  QtAndroid::runOnAndroidThread([on]{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (activity.isValid()) {
      QAndroidJniObject window =
          activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

      if (window.isValid()) {
        const int FLAG_KEEP_SCREEN_ON = 128;
        if (on) {
          window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        } else {
          window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
      }
    }
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
  });
}
#endif

ScreenTransition::ScreenTransition() :
    mScene(new QGraphicsScene(this))
{
    {
        QGLWidget *context=new QGLWidget(QGLFormat(QGL::SampleBuffers));
        //if OpenGL is present, use it
        if(context->isValid())
        {
            setViewport(context);
            setRenderHint(QPainter::Antialiasing,true);
            setRenderHint(QPainter::TextAntialiasing,true);
            setRenderHint(QPainter::HighQualityAntialiasing,true);
            setRenderHint(QPainter::SmoothPixmapTransform,false);
            setRenderHint(QPainter::NonCosmeticDefaultPen,true);
        }
        //else use the CPU only
    }
    multiplaySelected=false;
    mousePress=nullptr;
    m_backgroundStack=nullptr;
    m_foregroundStack=nullptr;
    m_aboveStack=nullptr;
    subserver=nullptr;
    characterList=nullptr;
    connexionManager=nullptr;
    ccmap=nullptr;
    overmap=nullptr;
    m=nullptr;
    o=nullptr;
    d=nullptr;
    /*solo=nullptr;*/
    multi=nullptr;
    login=nullptr;
    inGame=false;
    #ifdef CATCHCHALLENGER_SOLO
    internalServer=nullptr;
    #endif
    setBackground(&b);
    setForeground(&l);
    if(!connect(&l,&LoadingScreen::finished,this,&ScreenTransition::loadingFinished))
        abort();
    #ifndef NOTHREADS
    threadSolo=new QThread(this);
    #endif

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&ScreenTransition::render))
        abort();
    if(!connect(&timerUpdateFPS,&QTimer::timeout,this,&ScreenTransition::updateFPS))
        abort();
    timerUpdateFPS.start();

    setScene(mScene);
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState | QGraphicsView::IndirectPainting);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);
    viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    #ifdef Q_OS_ANDROID
    keep_screen_on(true);
    #endif

    imageText=new QGraphicsPixmapItem();
    mScene->addItem(imageText);
    imageText->setPos(0,0);
    imageText->setZValue(999);

    render();
    setTargetFPS(100);
}

ScreenTransition::~ScreenTransition()
{
    #ifndef NOTHREADS
    threadSolo->exit();
    threadSolo->wait();
    threadSolo->deleteLater();
    #endif
}

void ScreenTransition::resizeEvent(QResizeEvent *event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    (void)event;
}

void ScreenTransition::mousePressEvent(QMouseEvent *event)
{
    /*const QPointF p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
/*    std::cerr << "void ScreenTransition::mousePressEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->setPressed(true);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(p,temp,callParentClass);
        mousePress=nullptr;
    }
    bool temp=false;
    if(m_aboveStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_aboveStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!temp && m_foregroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_foregroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!temp && m_backgroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_backgroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!temp || callParentClass)
        QGraphicsView::mousePressEvent(event);
}

void ScreenTransition::mouseReleaseEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!pressValidated && m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated && m_backgroundStack!=nullptr)
    {
        m_backgroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!pressValidated || callParentClass)
        QGraphicsView::mouseReleaseEvent(event);
}

void ScreenTransition::mouseMoveEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    else if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated)
        QGraphicsView::mouseMoveEvent(event);
}

void ScreenTransition::closeEvent(QCloseEvent *event)
{
    std::cout << " ScreenTransition::closeEvent()" << std::endl;
    inGame=false;
    #ifdef CATCHCHALLENGER_SOLO
    if(internalServer!=nullptr)
    {
        if(!internalServer->isFinished())
        {
            //wait the server need be terminated
            std::cout << "ScreenTransition::closeEvent): internalServer!=nullptr" << std::endl;
            hide();
            if(connexionManager!=nullptr)
                if(connexionManager->client!=nullptr)
                    connexionManager->client->disconnectFromHost();
            ///internalServer->stop();
            event->accept();
            return;
        }
        else
            std::cout << "ScreenTransition::closeEvent: internalServer->isFinished()" << std::endl;
    }
    #endif
    QWidget::closeEvent(event);
    QCoreApplication::quit();
}

void ScreenTransition::setBackground(ScreenInput *widget)
{
    if(m_backgroundStack!=nullptr)
        mScene->removeItem(m_backgroundStack);
    m_backgroundStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_backgroundStack);
        m_backgroundStack->setZValue(-1);
    }
}

void ScreenTransition::setForeground(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
    if(m_foregroundStack!=nullptr)
        mScene->removeItem(m_foregroundStack);
    m_foregroundStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_foregroundStack);
    }
}

void ScreenTransition::setForegroundInGame(ScreenInput *widget)
{
    if(!inGame)
        return;
    setForeground(widget);
}

void ScreenTransition::setAbove(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
    if(m_aboveStack!=nullptr)
        mScene->removeItem(m_aboveStack);
    m_aboveStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_aboveStack);
    }
}

void ScreenTransition::loadingFinished()
{
    if(connexionManager==nullptr)
        toMainScreen();
    else if(connexionManager->client==nullptr)
        toMainScreen();
    else if(!connexionManager->client->getIsLogged())
        toMainScreen();
    else if(!connexionManager->client->getCaracterSelected())
        toSubServer();
    else
        toInGame();
}

void ScreenTransition::toSubServer()
{
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::toMainScreen))
            abort();
    }
    setForeground(subserver);
}

void ScreenTransition::toInGame()
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::toSubServer))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    setForeground(characterList);
}

void ScreenTransition::toMainScreen()
{
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(Audio::audio==nullptr)
    {
        AudioGL* a=new AudioGL();
        Audio::audio=a;
        if(!connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,a,&AudioGL::stopCurrentAmbiance,Qt::DirectConnection))
            abort();
    }
    #endif
    if(m==nullptr)
    {
        m=new MainScreen();
        if(!connect(m,&MainScreen::goToOptions,this,&ScreenTransition::openOptions))
            abort();
        if(!connect(m,&MainScreen::goToDebug,this,&ScreenTransition::openDebug))
            abort();
        if(!connect(m,&MainScreen::goToSolo,this,&ScreenTransition::openSolo))
            abort();
        if(!connect(m,&MainScreen::goToMulti,this,&ScreenTransition::openMulti))
            abort();
    }
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(QString::fromStdString(CatchChallenger::Version::str)));
}

void ScreenTransition::openOptions()
{
    if(o==nullptr)
    {
        o=new OptionsDialog();
        if(!connect(o,&OptionsDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(o);
}

void ScreenTransition::openDebug()
{
    if(d==nullptr)
    {
        d=new DebugDialog();
        if(!connect(d,&DebugDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(d);
}

void ScreenTransition::removeAbove()
{
    setAbove(nullptr);
}

void ScreenTransition::openSolo()
{
    multiplaySelected=false;
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
    QStringList l=QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    if(l.empty())
    {
        errorString(tr("No writable path").toStdString());
        return;
    }
    QString savegamesPath=l.first()+"/solo/";
    if(m!=nullptr)
        m->setError(std::string());
    #ifdef CATCHCHALLENGER_SOLO
    //locate the new folder and create it
    if(!QDir().mkpath(savegamesPath))
    {
        errorString(tr("Unable to write savegame into: %1").arg(savegamesPath).toStdString());
        return;
    }

    //initialize the db
    if(!QFile(savegamesPath+"catchchallenger.db.sqlite").exists())
    {
        QByteArray dbData;
        {
            QFile dbSource(QStringLiteral(":/catchchallenger.db.sqlite"));
            if(!dbSource.open(QIODevice::ReadOnly))
            {
                errorString(QStringLiteral("Unable to open the db model: %1").arg(savegamesPath).toStdString());
                CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
                return;
            }
            dbData=dbSource.readAll();
            if(dbData.isEmpty())
            {
                errorString(QStringLiteral("Unable to read the db model: %1").arg(savegamesPath).toStdString());
                CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
                return;
            }
            dbSource.close();
        }
        {
            QFile dbDestination(savegamesPath+"catchchallenger.db.sqlite");
            if(!dbDestination.open(QIODevice::WriteOnly))
            {
                errorString(QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath).toStdString());
                CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
                return;
            }
            if(dbDestination.write(dbData)<0)
            {
                dbDestination.close();
                errorString(QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath).toStdString());
                CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
                return;
            }
            dbDestination.close();
        }
    }

    if(!Settings::settings->contains("pass"))
    {
        //initialise the pass
        QString pass=QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",32));

        //initialise the meta data
        Settings::settings->setValue("pass",pass);
        Settings::settings->sync();
        if(Settings::settings->status()!=QSettings::NoError)
        {
            errorString(QStringLiteral("Unable to write savegame pass into: %1").arg(savegamesPath).toStdString());
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
    }

    if(internalServer!=nullptr)
    {
        internalServer->deleteLater();
        internalServer=nullptr;
    }
    if(internalServer==nullptr)
    {
        internalServer=new CatchChallenger::InternalServer();
        #ifndef NOTHREADS
        threadSolo->start();
        //internalServer->moveToThread(threadSolo);//-> Timers cannot be stopped from another thread, fixed by using QThread InternalServer::run()
        #endif
    }
    if(!connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&ScreenTransition::is_started,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(internalServer,&CatchChallenger::InternalServer::error,this,&ScreenTransition::errorString,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    toLoading("Open the local game");

    {
        //std::string datapackPathBase=client->datapackPathBase();
        std::string datapackPathBase=QCoreApplication::applicationDirPath().toStdString()+"/datapack/";
        CatchChallenger::GameServerSettings formatedServerSettings=internalServer->getSettings();

        CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=0;
        CommonSettingsCommon::commonSettingsCommon.max_character=1;
        CommonSettingsCommon::commonSettingsCommon.min_character=1;

        formatedServerSettings.automatic_account_creation=true;
        formatedServerSettings.max_players=99;//> 1 to allow open to lan
        formatedServerSettings.sendPlayerNumber = true;// to work with open to lan
        //formatedServerSettings.compressionType=CompressionProtocol::CompressionType::None;
        formatedServerSettings.compressionType=CompressionProtocol::CompressionType::Zstandard;// to allow open to lan
        formatedServerSettings.everyBodyIsRoot                                      = true;
        formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                       = true;

        formatedServerSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_login.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_base.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_base.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_common.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_server.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        //formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_None;
        formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_Simple;// to allow open to lan
        formatedServerSettings.datapack_basePath=datapackPathBase;

        {
            CatchChallenger::GameServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["day"];
            event.cycle=60;
            event.offset=0;
            event.value="day";
        }
        {
            CatchChallenger::GameServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["night"];
            event.cycle=60;
            event.offset=30;
            event.value="night";
        }
        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(list.isEmpty())
            {
                errorString((tr("No main code detected into the current datapack, base, check: ")+QString::fromStdString(datapackPathBase)+"/map/main/").toStdString());
                return;
            }
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
        }
        QDir mainDir(QString::fromStdString(datapackPathBase)+"map/main/"+
                     QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/");
        if(!mainDir.exists())
        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(list.isEmpty())
            {
                errorString((tr("No main code detected into the current datapack, empty main, check: ")+QString::fromStdString(datapackPathBase)+"/map/main/").toStdString());
                return;
            }
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
        }

        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+
                                           "/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(!list.isEmpty())
                CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
            else
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            QDir subDir(QString::fromStdString(datapackPathBase)+"/map/main/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+"/");
            if(!subDir.exists())
            {
                const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/"+
                                               QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                        .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
                if(!list.isEmpty())
                    CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
                else
                    CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
            }
        }

        internalServer->setSettings(formatedServerSettings);
    }

    internalServer->start();
    //do after server is init connectToServer(connexionInfo,QString(),QString());
    #endif
}

void ScreenTransition::is_started(bool started)
{
    std::cout << "ScreenTransition::is_started(" << started << ")" << std::endl;
    #ifdef CATCHCHALLENGER_SOLO
    if(started)
    {
        ConnexionInfo connexionInfo;
        connexionInfo.connexionCounter=0;
        connexionInfo.isCustom=false;
        connexionInfo.port=0;
        connexionInfo.lastConnexion=0;
        connexionInfo.proxyPort=0;
        connectToServer(connexionInfo,"Admin",Settings::settings->value("pass").toString());
        return;
    }
    else
    {
        if(internalServer!=nullptr)
        {
            delete internalServer;
            internalServer=nullptr;
        }
        std::cout << "ScreenTransition::is_started(" << started << ") isVisible(): " << isVisible() << std::endl;
        if(!isVisible())
            QCoreApplication::quit();
    }
    #endif
}

void ScreenTransition::openMulti()
{
    multiplaySelected=true;
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
    if(m!=nullptr)
        m->setError(std::string());
    if(multi==nullptr)
    {
        multi=new Multi();
        if(!connect(multi,&Multi::backMain,this,&ScreenTransition::backMain))
            abort();
        if(!connect(multi,&Multi::connectToServer,this,&ScreenTransition::connectToServer))
            abort();
    }
    setForeground(multi);
}

void ScreenTransition::connectToServer(ConnexionInfo connexionInfo,QString login,QString pass)
{
    Q_UNUSED(connexionInfo);
    Q_UNUSED(login);
    Q_UNUSED(pass);
    setForeground(&l);
    //baseWindow=new CatchChallenger::BaseWindow();
    if(connexionManager!=nullptr)
        delete connexionManager;
    //work around for the resetAll() of protocol
    const std::string mainDatapackCode=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
    const std::string subDatapackCode=CommonSettingsServer::commonSettingsServer.subDatapackCode;
    connexionManager=new ConnexionManager(&l);
    connexionManager->connectToServer(connexionInfo,login,pass);
    CommonSettingsServer::commonSettingsServer.mainDatapackCode=mainDatapackCode;
    CommonSettingsServer::commonSettingsServer.subDatapackCode=subDatapackCode;
    if(!connect(connexionManager,&ConnexionManager::logged,this,&ScreenTransition::logged))
        abort();
    if(!connect(connexionManager,&ConnexionManager::errorString,this,&ScreenTransition::errorString))
        abort();
    if(!connect(connexionManager,&ConnexionManager::disconnectedFromServer,this,&ScreenTransition::disconnectedFromServer,Qt::QueuedConnection))
        abort();
    if(!connect(connexionManager,&ConnexionManager::goToMap,this,&ScreenTransition::goToMap))
        abort();
    l.progression(0,100);
}

/*void ScreenTransition::errorString(std::string error)
{
    setForeground(m);
    m->setError(error);
}*/

void ScreenTransition::toLoading(QString text)
{
    if(m_foregroundStack!=&l)
        l.progression(0,100);
    l.setText(text);
    setBackground(&b);
    setForeground(&l);
}

void ScreenTransition::backMain()
{
    setForeground(m);
}

void ScreenTransition::backSubServer()
{
    if(connexionManager->client->getServerOrdenedList().size()<2)
    {
        if(multiplaySelected && multi!=nullptr)
            setForeground(multi);
        else
            setForeground(m);
    }
    else
        setForeground(subserver);
}

void ScreenTransition::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    this->characterEntryList=characterEntryList;
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::backMain))
            abort();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
    }
    setForeground(subserver);
    //after setForeground to be able to change Foreground
    subserver->logged(connexionManager->client->getServerOrdenedList(),connexionManager);
}

void ScreenTransition::connectToSubServer(const int indexSubServer)
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    setForeground(characterList);
    //after setForeground to be able to change Foreground
    characterList->connectToSubServer(indexSubServer,connexionManager,characterEntryList);

    //todo: optimise by prevent create each time
    if(ccmap!=nullptr)
    {
        delete ccmap;
        ccmap=nullptr;
    }
    if(ccmap==nullptr)
    {
        ccmap=new CCMap();
        /*if(!connect(ccmap,&CCMap::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(ccmap,&CCMap::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();*/
        if(!connect(ccmap,&CCMap::error,this,&ScreenTransition::errorString))
            abort();
    }
    ccmap->setVar(connexionManager);

    if(overmap!=nullptr)
    {
        delete overmap;
        overmap=nullptr;
    }
    if(overmap==nullptr)
    {
        overmap=new OverMapLogic();
        if(!connect(overmap,&OverMapLogic::error,this,&ScreenTransition::errorString))
            abort();
    }
    overmap->resetAll();
    overmap->setVar(ccmap,connexionManager);
    overmap->connectAllSignals();
}

void ScreenTransition::selectCharacter(const int indexSubServer,const int indexCharacter)
{
    l.reset();
    l.progression(0,100);
    l.setText("Selecting your character...");
    setAbove(nullptr);
    setForeground(&l);
    connexionManager->selectCharacter(indexSubServer,indexCharacter);
}

void ScreenTransition::disconnectedFromServer()
{
    inGame=false;
    #ifdef CATCHCHALLENGER_SOLO
    if(internalServer!=nullptr)
        internalServer->stop();
    #endif
    //baseWindow->resetAll();
    setBackground(&b);
    setForeground(m);
    setAbove(nullptr);
}

void ScreenTransition::errorString(std::string error)
{
    inGame=false;
    if(connexionManager!=nullptr)
        if(connexionManager->client!=nullptr)
            connexionManager->client->disconnectFromHost();
    #ifdef CATCHCHALLENGER_SOLO
    if(internalServer!=nullptr)
    {
        std::cerr << "ScreenTransition::errorString(): internalServer!=nullptr" << std::endl;
        internalServer->stop();
    }
    #endif
    if(!error.empty())
    {
        std::cerr << "ScreenTransition::errorString(" << error << ")" << std::endl;
        setBackground(&b);
        setForeground(m);
        setAbove(nullptr);
    }
    if(m!=nullptr)
        m->setError(error);
}

void ScreenTransition::goToServerList()
{
    setBackground(&b);
    //setForeground(baseWindow);
    setAbove(nullptr);
}

void ScreenTransition::goToMap()
{
    inGame=true;
    setBackground(ccmap);
    setForeground(overmap);
    setAbove(nullptr);
}

void ScreenTransition::render()
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    //mScene->update();
    viewport()->update();
}

void ScreenTransition::paintEvent(QPaintEvent * event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    timeRender.restart();

    QGraphicsView::paintEvent(event);

    uint32_t elapsed=timeRender.elapsed();
    if(waitRenderTime<=elapsed)
        timerRender.start(1);
    else
        timerRender.start(waitRenderTime-elapsed);

    if(frameCounter<65535)
        frameCounter++;
}

void ScreenTransition::updateFPS()
{
    const unsigned int FPS=(int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed();
    emit newFPSvalue(FPS);
    QImage pix(50,40,QImage::Format_ARGB32_Premultiplied);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setFont(QFont("Times", 12, QFont::Bold));

    const int offset=1;
    p.setPen(QPen(Qt::black));
    p.drawText(pix.rect().x()+offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()+offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));

    p.setPen(QPen(Qt::white));
    p.drawText(pix.rect(), Qt::AlignCenter,QString::number(FPS));
    imageText->setPixmap(QPixmap::fromImage(pix));

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

void ScreenTransition::setTargetFPS(int targetFPS)
{
    if(targetFPS==0)
        waitRenderTime=0;
    else
    {
        waitRenderTime=static_cast<uint8_t>(static_cast<float>(1000.0)/(float)targetFPS);
        if(waitRenderTime<1)
            waitRenderTime=1;
    }
}

void ScreenTransition::keyPressEvent(QKeyEvent * event)
{
    bool eventTriggerGeneral=false;
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted())
        QGraphicsView::keyPressEvent(event);
}

void ScreenTransition::keyReleaseEvent(QKeyEvent *event)
{
    bool eventTriggerGeneral=false;
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted())
        QGraphicsView::keyReleaseEvent(event);
}
