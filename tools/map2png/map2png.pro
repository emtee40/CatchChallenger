TEMPLATE = app
TARGET = map2png
DEFINES += MAXIMIZEPERFORMANCEOVERDATABASESIZE

QMAKE_CXXFLAGS+="-fstack-protector-all -std=c++0x -g"

QT += xml network opengl

LIBS += -ltiled
INCLUDEPATH += /usr/include/libtiled/

DEFINES += ONLYMAPRENDER NOWEBSOCKET

include(../../general/general.pri)

win32:RC_FILE += resources-windows.rc

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

SOURCES += main.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/DatapackClientLoader.cpp \
    ../../client/libqtcatchchallenger/Language.cpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoader.cpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp \
    ../../client/libqtcatchchallenger/Settings.cpp \
    MapDoor.cpp \
    Map_client.cpp \
    TriggerAnimation.cpp \
    map2png.cpp

HEADERS += map2png.h \
    ../../client/libcatchchallenger/DatapackChecksum.hpp \
    ../../client/libcatchchallenger/DatapackClientLoader.hpp \
    ../../client/libqtcatchchallenger/Language.hpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp \
    ../../client/libqtcatchchallenger/QtDatapackClientLoaderThread.hpp \
    ../../client/libqtcatchchallenger/Settings.hpp \
    MapDoor.hpp \
    Map_client.hpp \
    TriggerAnimation.hpp

RESOURCES += resources.qrc

win32:RESOURCES += $$PWD/../../general/base/resources/resources-windows-qt-plugin.qrc

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
$$PWD/../../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../../general/tinyXML2/tinyxml2c.cpp

#linux:QMAKE_LFLAGS += -fuse-ld=mold
#linux:LIBS += -fuse-ld=mold
#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#
