#ifndef CATCHCHALLENGER_QTDATAPACKCHECKSUM_H
#define CATCHCHALLENGER_QTDATAPACKCHECKSUM_H

#include <vector>
#include <string>
#include <QThread>
#include "../libcatchchallenger/DatapackChecksum.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
class DLL_PUBLIC QtDatapackChecksum :
        #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
        public QThread
        #else
        public QObject
        #endif
        , public DatapackChecksum
{
    Q_OBJECT
public:
    explicit QtDatapackChecksum();
    ~QtDatapackChecksum();
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    void stopThread();
    static QThread thread;
private:
    #endif
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
public slots:
    void doDifferedChecksumBase(const std::string &datapackPath);
    void doDifferedChecksumMain(const std::string &datapackPath);
    void doDifferedChecksumSub(const std::string &datapackPath);
signals:
    void datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    #endif
};
}

#endif // CATCHCHALLENGER_QTDATAPACKCHECKSUM_H
