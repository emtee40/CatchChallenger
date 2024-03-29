#ifndef CATCHCHALLENGER_QTCLIENT_H
#define CATCHCHALLENGER_QTCLIENT_H

#include <QIODevice>

namespace CatchChallenger {
class QtClient
{
public:
    QtClient(QIODevice *qtSocket);
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    bool disconnectClientTimer();
    bool disconnectClient();
    void closeSocket();
    bool isValid();
public:
    QIODevice *socket;
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
