include(../general/general.pri)
include(catchchallenger-server-qt.pri)

#choose one of:
DEFINES += CATCHCHALLENGER_XLMPARSER_TINYXML2 NOWEBSOCKET

HEADERS += $$PWD/../general/tinyXML2/tinyxml2.hpp
SOURCES += $$PWD/../general/tinyXML2/tinyxml2.cpp \
$$PWD/../general/tinyXML2/tinyxml2b.cpp \
$$PWD/../general/tinyXML2/tinyxml2c.cpp
