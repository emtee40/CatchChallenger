#QMAKE_CFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"
#QMAKE_CXXFLAGS+="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops -ffast-math -fno-rtti"

#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion -Wno-disabled-macro-expansion"
#QMAKE_CXXFLAGS+="-Wno-weak-vtables -Wno-non-virtual-dtor -Wno-gnu-statement-expression -Wno-implicit-fallthrough -Wno-float-equal -Wno-unreachable-code -Wno-missing-noreturn -Wno-unreachable-code-return -Wno-vla-extension -Wno-format-nonliteral -Wno-vla -Wno-embedded-directive -Wno-missing-variable-declarations -Wno-missing-braces -Wno-disabled-macro-expansion"

QMAKE_CFLAGS+="-fstack-protector-all -g -fno-rtti"
QMAKE_CXXFLAGS+="-fstack-protector-all -g -fno-rtti"
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wall -Wextra"

QT       -= gui widgets sql xml network core
TEMPLATE = app
#DEFINES += SERVERSSL

DEFINES += EPOLLCATCHCHALLENGERSERVER QT_NO_EMIT
#DEFINES += EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
DEFINES += CATCHCHALLENGER_CLASS_GATEWAY CATCHCHALLENGER_DB_BLACKHOLE
DEFINES += DEBUG_PROTOCOLPARSING_RAW_NETWORK PROTOCOLPARSINGINPUTOUTPUTDEBUG

LIBS += -lcurl

CONFIG += c++11

TARGET = catchchallenger-gateway
CONFIG   += console

TEMPLATE = app

SOURCES += \
    ../../general/base/CompressionProtocol.cpp \
    ../../general/sha224/sha224.cpp \
    EpollClientLoginSlave.cpp \
    EpollServerLoginSlave.cpp \
    EpollClientLoginSlaveStaticVar.cpp \
    EpollClientLoginSlaveProtocolParsing.cpp \
    EpollClientLoginSlaveWrite.cpp \
    LinkToGameServerStaticVar.cpp \
    LinkToGameServerProtocolParsing.cpp \
    LinkToGameServer.cpp \
    TimerDdos.cpp \
    DatapackDownloaderBase.cpp \
    DatapackDownloader_sub.cpp \
    DatapackDownloader_main.cpp \
    DatapackDownloaderMainSub.cpp \
    EpollClientLoginSlaveDatapack.cpp \
    FacilityLibGateway.cpp \
    main-epoll-gateway.cpp \
    ../epoll/Epoll.cpp \
    ../epoll/EpollGenericSslServer.cpp \
    ../epoll/EpollGenericServer.cpp \
    ../epoll/EpollClient.cpp \
    ../epoll/EpollSocket.cpp \
    ../epoll/EpollSslClient.cpp \
    ../epoll/EpollClientToServer.cpp \
    ../epoll/EpollSslClientToServer.cpp \
    ../epoll/EpollTimer.cpp \
    ../../client/libcatchchallenger/DatapackChecksum.cpp \
    ../../client/libcatchchallenger/TarDecode.cpp \
    ../../general/base/FacilityLibGeneral.cpp \
    ../../general/base/ProtocolParsingCheck.cpp \
    ../../general/base/ProtocolParsingGeneral.cpp \
    ../../general/base/ProtocolParsingInput.cpp \
    ../../general/base/ProtocolParsingOutput.cpp \
    ../../general/base/CommonSettingsCommon.cpp \
    ../../general/base/CommonSettingsServer.cpp \
    ../../general/base/cpp11addition.cpp \
    ../../general/base/cpp11additionstringtointc.cpp \
    ../../general/base/cpp11additionstringtointcpp.cpp \
    ../base/TinyXMLSettings.cpp

HEADERS += \
    ../../general/base/CompressionProtocol.hpp \
    ../../general/sha224/sha224.hpp \
    EpollClientLoginSlave.h \
    EpollServerLoginSlave.h \
    LinkToGameServer.h \
    TimerDdos.h \
    DatapackDownloaderBase.h \
    DatapackDownloaderMainSub.h \
    FacilityLibGateway.h \
    ../epoll/Epoll.h \
    ../epoll/EpollGenericSslServer.h \
    ../epoll/EpollGenericServer.h \
    ../epoll/EpollClient.h \
    ../epoll/EpollSocket.h \
    ../epoll/EpollSslClient.h \
    ../epoll/EpollClientToServer.h \
    ../epoll/EpollSslClientToServer.h \
    ../epoll/EpollTimer.h \
    ../epoll/BaseClassSwitch.h \
    ../../general/base/CommonSettingsCommon.h \
    ../../general/base/CommonSettingsServer.h \
    ../../general/base/GeneralStructures.h \
    ../../general/base/FacilityLibGeneral.h \
    ../../general/base/ProtocolParsing.h \
    ../../general/base/ProtocolParsingCheck.h \
    ../../general/base/GeneralVariable.h \
    ../../client/libcatchchallenger/DatapackChecksum.h \
    ../../client/libcatchchallenger/TarDecode.h \
    ../base/TinyXMLSettings.h \
    ../VariableServer.h

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2

HEADERS += $$PWD/../../general/tinyXML2/tinyxml2.h
SOURCES += $$PWD/../../general/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/tinyXML2/tinyxml2c.cpp

LIBS += -lxxhash -lzstd

linux:QMAKE_LFLAGS += -fuse-ld=mold
linux:LIBS += -fuse-ld=mold
#precompile_header:!isEmpty(PRECOMPILED_HEADER) {
#DEFINES += USING_PCH
#
