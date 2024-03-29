DEFINES += CATCHCHALLENGER_NOAUDIO

include(../../general/general.pri)
include(../../general/tinyXML2/tinyXML2.pri)
include(../../general/tinyXML2/tinyXML2header.pri)
include(../../client/libcatchchallenger/lib.pri)
include(../../client/libcatchchallenger/libheader.pri)
include(../../client/libqtcatchchallenger/libqt.pri)
include(../../client/libqtcatchchallenger/libqtheader.pri)
include(../bot/actions/Actions.pri)

QT       += core gui xml network sql
QT += widgets websockets

TARGET = bot-actions
TEMPLATE = app

DEFINES += BOTACTIONS CATCHCHALLENGER_BOT MAXIMIZEPERFORMANCEOVERDATABASESIZE CATCHCHALLENGER_ABORTIFERROR

SOURCES += main.cpp\
    ../../client/qtcpu800x600/base/ChatParsing.cpp \
    GenerateMapZone.cpp \
        MainWindow.cpp \
    ../bot/MultipleBotConnection.cpp \
    ../bot/MultipleBotConnectionImplForGui.cpp \
    ../bot/BotInterface.cpp \
    BotTargetList.cpp \
    BotTargetListGUI.cpp \
    BotTargetListNear.cpp \
    MapBrowse.cpp \
    WaitScreen.cpp \
    BotTargetListPlayerInfo.cpp \
    BotTargetListNextPos.cpp \
    SocialChat.cpp \
    TargetFilter.cpp \
    MultipleBotConnectionAction.cpp \
    DatabaseBot.cpp
HEADERS  += MainWindow.h \
    ../../client/qtcpu800x600/base/ChatParsing.hpp \
    ../bot/MultipleBotConnection.h \
    ../bot/MultipleBotConnectionImplForGui.h \
    ../bot/BotInterface.h \
    BotTargetList.h \
    MapBrowse.h \
    WaitScreen.h \
    SocialChat.h \
    TargetFilter.h \
    MultipleBotConnectionAction.h \
    DatabaseBot.h

FORMS    += MainWindow.ui \
    BotTargetList.ui \
    MapBrowse.ui \
    WaitScreen.ui \
    SocialChat.ui \
    TargetFilter.ui

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

RESOURCES += \
    localr.qrc

#QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
#QMAKE_CFLAGS+="-Wall -Wextra -fno-omit-frame-pointer -fsanitize=address"
#QMAKE_LFLAGS+="-fno-omit-frame-pointer -Wl,--no-undefined -fsanitize=address"
#/usr/lib64/qt5/bin/qmake -spec linux-clang
