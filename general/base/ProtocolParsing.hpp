#ifndef CATCHCHALLENGER_PROTOCOLPARSING_H
#define CATCHCHALLENGER_PROTOCOLPARSING_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <string>

#include "GeneralVariable.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include "CompressionProtocol.hpp"
#endif
#include "lib.h"

#define CATCHCHALLENGER_COMMONBUFFERSIZE 4096

#ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
// without datapack send
#define CATCHCHALLENGER_BIGBUFFERSIZE 256*1024
#else
#define CATCHCHALLENGER_BIGBUFFERSIZE 16*1024*1024
#endif
#if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_MAX_PACKET_SIZE
#error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_MAX_PACKET_SIZE
#endif

#ifdef EPOLLCATCHCHALLENGERSERVER
    #ifndef CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER
    #define CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER CATCHCHALLENGER_BIGBUFFERSIZE-16
    #endif
    #ifndef CATCHCHALLENGER_EXTRA_CHECK
        #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
            #define CATCHCHALLENGERSERVERDROPIFCLENT
        #endif
    #endif
#endif

#define CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT 0x7F
#define CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER 0x01

#if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLASS_GATEWAY)
/// \warning can't be static where open multiple connexion to game server as:
/// - login in proxy mode
/// - gateway
#define DYNAMICPACKETFIXEDSIZE
#endif

namespace CatchChallenger {

#if ! defined (ONLYMAPRENDER)
class ProtocolParsingCheck;

class DLL_PUBLIC ProtocolParsing
{
public:
    /// \brief Define the mode of transmission: client or server
    enum PacketModeTransmission
    {
        PacketModeTransmission_Server=0x00,
        PacketModeTransmission_Client=0x01
    };
    enum InitialiseTheVariableType
    {
        AllInOne,
        LoginServer,
        GameServer,
        MasterServer
    };
    /// \warning can't be static where open multiple connexion to game server as:
    /// - login in proxy mode
    /// - gateway
    #ifndef DYNAMICPACKETFIXEDSIZE
    static uint8_t packetFixedSize[256+128];
    #else
    static uint8_t packetFixedSize8[256+128];
    static uint8_t packetFixedSize16[256+128];
    uint8_t *packetFixedSize;
    #endif

    ProtocolParsing();
    static void initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType=InitialiseTheVariableType::AllInOne);
    #ifndef DYNAMICPACKETFIXEDSIZE
    static void setMaxPlayers(const uint16_t &maxPlayers);
    #else
    virtual void setMaxPlayers(const uint16_t &maxPlayers);
    #endif
    static int32_t decompressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    static int32_t compressZstandard(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
protected:
    virtual void errorParsingLayer(const std::string &error) = 0;
    virtual void messageParsingLayer(const std::string &message) const = 0;
private:
    virtual void reset() = 0;
};

class DLL_PUBLIC ProtocolParsingBase : public ProtocolParsing
{
public:
    ProtocolParsingBase(
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    PacketModeTransmission packetModeTransmission
    #endif
    );
    virtual ~ProtocolParsingBase();
    friend class ProtocolParsing;
    friend class ProtocolParsingCheck;
    virtual ssize_t readFromSocket(char * data, const size_t &size) = 0;
    virtual ssize_t writeToSocket(const char * const data, const size_t &size) = 0;
    virtual void registerOutputQuery(const uint8_t &queryNumber, const uint8_t &packetCode) = 0;
public:
    //this interface allow 0 copy method, return 1 if all is ok, return 0 if need more data, -1 if critical error and need disconnect
    int8_t parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::vector<std::string> getQueryRunningList();
    #endif
protected:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    #if defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
    //internal fast path to boost the move on map performance
    virtual void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction) = 0;
    #endif
    #endif
    int8_t parseHeader(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor);
    int8_t parseQueryNumber(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    int8_t parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    int8_t parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor);
    bool parseDispatch(const char * const data,const int &size);
    inline bool isReply() const;
    virtual void breakNeedMoreData();
    std::vector<char> header_cut;
    uint8_t flags;
    /* flags & 0x80 = have header
     * flags & 0x40 = have data size
     * flags & 0x20 = have query number
     #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
     * flags & 0x10 = isClient
     #endif
     * flags & 0x08 = allowDynamicSize
    */
protected:
    //have message without reply
    virtual bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size) = 0;
    //have query with reply
    virtual bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;
    //send reply
    virtual bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) = 0;

    virtual void reset();
    virtual void resetForReconnect();
private:
    std::vector<char> dataToWithoutHeader;
    uint32_t dataSize;
    // function
    void dataClear();
public:
    //reply to the query
    char outputQueryNumberToPacketCode[CATCHCHALLENGER_MAXPROTOCOLQUERY];//invalidation packet code: 0x00, store the packetCode
    char inputQueryNumberToPacketCode[CATCHCHALLENGER_MAXPROTOCOLQUERY];//invalidation packet code: 0x00, store the packetCode, store size is useless because the resolution or is do at send or at receive, then no performance gain

    static char tempBigBufferForOutput[CATCHCHALLENGER_BIGBUFFERSIZE];
    static char tempBigBufferForInput[CATCHCHALLENGER_BIGBUFFERSIZE];//to store the input buffer on linux READ() interface or with Qt
private:
    bool internalPackOutcommingData(const char * const data,const int &size);

    // for data
    uint8_t packetCode;
    uint8_t queryNumber;
public:
    virtual void storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber);
    bool internalSendRawSmallPacket(const char * const data,const int &size);
protected:
    //reply to the query
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    virtual bool disconnectClient() = 0;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    virtual CompressionProtocol::CompressionType getCompressType() const = 0;
    #endif
};

class DLL_PUBLIC ProtocolParsingInputOutput : public ProtocolParsingBase
{
public:
    ProtocolParsingInputOutput(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const PacketModeTransmission &packetModeTransmission
        #endif
       );
    virtual ~ProtocolParsingInputOutput();
    //friend class Client;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    uint64_t getTXSize() const;
    uint64_t getRXSize() const;
    #endif
    void storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber) override;
    bool haveInputQuery(const uint8_t &queryNumber) const;
    virtual void registerOutputQuery(const uint8_t &queryNumber, const uint8_t &packetCode) override;

    virtual void closeSocket() = 0;
    void resetForReconnect() override;
    #ifdef DYNAMICPACKETFIXEDSIZE
    void setMaxPlayers(const uint16_t &maxPlayers) override;
    #endif
protected:
    /*virtual for void LinkToGameServer::parseIncommingData()
    of gateway, readTheFirstSslHeader() */virtual void parseIncommingData();
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CompressionProtocol::CompressionType getCompressType() const override;
    #endif
    ssize_t write(const char * const data, const size_t &size);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ProtocolParsingCheck *protocolParsingCheck;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    uint64_t TXSize;
    uint64_t RXSize;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    char commonBuffer[CATCHCHALLENGER_COMMONBUFFERSIZE];
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    int parseIncommingDataCount;//by object
    #endif
};
#endif

}

#endif // PROTOCOLPARSING_H
