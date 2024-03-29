#ifndef LOGINLINKTOGameServer_H
#define LOGINLINKTOGameServer_H

#include "../../general/base/ProtocolParsing.hpp"
#include "../epoll/EpollClient.hpp"
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

namespace CatchChallenger {
class EpollClientLoginSlave;
class LinkToGameServer : public EpollClient, public ProtocolParsingInputOutput
{
public:
    explicit LinkToGameServer(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    ~LinkToGameServer();
    enum Stat : uint8_t
    {
        Unconnected,
        Connecting,
        Connected,
        ProtocolGood,
        LoginInProgress,
        Logged,
    };
    Stat stat;

    //to delete into the event loop (main-epoll-login-slave.cpp) after unregister for epoll, into the next event loop parse
    static std::vector<void *> gameLinkToDelete[16];
    static size_t gameLinkToDeleteSize;
    static uint8_t gameLinkToDeleteIndex;
    static std::unordered_set<void *> detectDuplicateGameLinkToDelete;

    uint64_t get_lastActivity() const;
    uint64_t lastActivity;
    static uint64_t msFrom1970();//ms from 1970

    char tokenForGameServer[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    EpollClientLoginSlave *client;
    bool haveTheFirstSslHeader;
    static unsigned char protocolHeaderToMatchGameServer[2+5];
    uint8_t queryIdToReconnect;

    void setConnexionSettings();
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    bool trySelectCharacter(void * const client,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId);
    void sendProtocolHeader();
    void readTheFirstSslHeader();
    bool sendRawBlock(const char * const data,const unsigned int &size);
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    bool disconnectClient();
    ssize_t readFromSocket(char * data, const size_t &size);
    ssize_t writeToSocket(const char * const data, const size_t &size);
    void closeSocket();
protected:
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const std::string &errorString);

    //have message without reply
    bool parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    bool parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
private:
    int socketFd;
};
}

#endif // LOGINLINKTOGameServer_H
