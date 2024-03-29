#include "LinkToLogin.h"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/sha224/sha224.hpp"
#include "../../server/epoll/Epoll.hpp"
#include "../../server/epoll/EpollSocket.hpp"
#include "EpollServerStats.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <unistd.h>
#include <time.h>
#include <cstring>

using namespace CatchChallenger;

LinkToLogin *LinkToLogin::linkToLogin=NULL;
bool LinkToLogin::haveTheFirstSslHeader=false;
char LinkToLogin::host[]="localhost";
uint16_t LinkToLogin::port=22222;

LinkToLogin::LinkToLogin(
        #ifdef SERVERSSL
            SSL_CTX *ctx
        #endif
        ) :
        EpollClient(-1),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Client
            #endif
            ),
        stat(Stat::Unconnected),
        tryInterval(5),
        considerDownAfterNumberOfTry(3)
{
    flags|=0x08;
    rng.seed(time(0));
    queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
    warningLogicalGroupOutOfBound=false;
}

LinkToLogin::~LinkToLogin()
{
    closeSocket();
    //critical bug, need be reopened, never closed
    abort();
}

void LinkToLogin::displayErrorAndQuit(const char * errorString)
{
    #ifndef STATSODROIDSHOW2
    removeJsonFile();
    #else
    LinkToLogin::writeData("\ec\e[2s\e[1r\e[31m");
    LinkToLogin::writeData(errorString);
    #endif
    std::cerr << errorString << std::endl;
    abort();
}

const std::string &LinkToLogin::getJsonFileContent() const
{
    return jsonFileContent;
}

bool LinkToLogin::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }
    strcpy(LinkToLogin::host,host);
    LinkToLogin::port=port;
    if(infd!=-1)
    {
        std::cout << "close fd: " << infd << " " << __FILE__ << ":" << __LINE__ << std::endl;
        ::close(infd);
    }
    infd=-1;

    const int &socketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd<0)
    {
        std::cerr << "ERROR opening socket to login server (abort)" << std::endl;
        abort();
    }
    //resolv again the dns
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(host,std::to_string(port).c_str(), &hints, &result);
    if (s != 0) {
        std::cerr << "ERROR connecting to login server server on: " << host << ":" << port << ": " << gai_strerror(s) << std::endl;
        return false;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ..." << std::endl;
        int connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
        std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ... 0" << std::endl;
        if(connStatusType<0)
        {
            std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ... 1" << std::endl;
            unsigned int index=0;
            while(index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ... 2" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
                auto start = std::chrono::high_resolution_clock::now();
                connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end-start;
                index++;
                if(elapsed.count()<(uint32_t)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
                {
                    const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                }
                std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ... 3" << std::endl;
            }
            std::cout << "Try connect to login server host: " << host << ", port: " << std::to_string(port) << " ... 4" << std::endl;
        }
        if(connStatusType>=0)
        {
            std::cout << "Connected to login server" << std::endl;
            infd=sfd;
            {
                epoll_event event;
                event.data.ptr = this;
                event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                errno=0;
                int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                if(s == -1)
                {
                    std::cerr << __FILE__ << ":" << __LINE__ << " epoll_ctl on socket (login link) error, errno: " << errno << std::endl;
                    abort();
                }
            }
            haveTheFirstSslHeader=false;
            freeaddrinfo(result);
            if(infd==-1)
            {
                std::cerr << "infd==-1 internal error (abort)" << std::endl;
                abort();
            }
            return true;
        }
        std::cout << "close sfd: " << infd << __FILE__ << ":" << __LINE__ << std::endl;
        ::close(sfd);
    }
    if (rp == NULL)               /* No address succeeded */
        std::cerr << "ERROR No address succeeded, connecting to login server server on: " << host << ":" << port << std::endl;
    else
        std::cerr << "ERROR connecting to login server server on: " << host << ":" << port << std::endl;
    freeaddrinfo(result);           /* No longer needed */

    haveTheFirstSslHeader=false;
    if(infd>=0)
    {
        epoll_event event;
        event.data.ptr = this;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
        errno=0;
        int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
        if(s == -1)
        {
            std::cerr << __FILE__ << ":" << __LINE__ << " epoll_ctl on socket (login link) error, errno: " << errno << ", infd: " << infd << std::endl;
            abort();
        }
        if(infd==-1)
        {
            std::cerr << "infd==-1 internal error (abort)" << std::endl;
            abort();
        }
        return true;
    }
    else
        return false;
}

void LinkToLogin::setConnexionSettings(const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(tryInterval>0 && tryInterval<60)
        this->tryInterval=tryInterval;
    if(considerDownAfterNumberOfTry>0 && considerDownAfterNumberOfTry<60)
        this->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    if(infd==-1)
    {
        std::cerr << "LoginLinkToLogin::setConnexionSettings() LoginLinkToLogin::linkToLoginSocketFd==-1 (abort)" << std::endl;
        //abort();
    }
}

void LinkToLogin::connectInternal()
{
    /*Non sens for me, already done into tryConnect() LinkToLogin::linkToLoginSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToLogin::linkToLoginSocketFd<0)
    {
        std::cerr << "ERROR opening socket to login server (abort)" << std::endl;
        abort();
    }*/

    bool connStatusType=tryConnect(LinkToLogin::host,LinkToLogin::port,1,1);
    if(!connStatusType)
    {
        stat=Stat::Unconnected;
        return;
    }
    haveTheFirstSslHeader=false;
    if(connStatusType)
    {
        if(infd==-1)
        {
            std::cerr << "infd==-1 internal error (abort)" << std::endl;
            abort();
        }
        stat=Stat::Connected;
        std::cout << "(Re)Connected to login" << std::endl;
        if(!EpollServerStats::epollServerStats.reopen())
            std::cout << "Reopen unix socket failed: " << errno << " " << __FILE__ << ":" << __LINE__ << std::endl;
        setConnexionSettings(this->tryInterval,this->considerDownAfterNumberOfTry);
    }
    /*why disabled?: else
    {
        stat=Stat::Connecting;
        std::cout << "(Re)Connecting in progress to login" << std::endl;
    }*/
}

void LinkToLogin::readTheFirstSslHeader()
{
    if(haveTheFirstSslHeader)
        return;
    //std::cout << "LoginLinkToLogin::readTheFirstSslHeader()" << std::endl;
    char buffer[1];
    if(::read(infd,buffer,1)<0)
    {
        /*then disable:
        jsonFileContent.clear();
        displayErrorAndQuit("ERROR reading from socket to login server (abort)");*/
    }
    else if(infd>=0)
    {
        //reconnect, need maybe more time
        EpollSocket::make_non_blocking(infd);
    }
    #ifdef SERVERSSL
    if(buffer[0]!=0x01)
    {
        std::cerr << "ERROR server configured in ssl mode but protocol not done" << std::endl;
        abort();
    }
    #else
    if(buffer[0]!=0x00)
    {
        std::cerr << "ERROR server configured in clear mode but protocol not done" << std::endl;
        abort();
    }
    #endif
    haveTheFirstSslHeader=true;
    stat=Stat::Connected;
    EpollSocket::make_non_blocking(infd);
    sendProtocolHeader();
}

bool LinkToLogin::disconnectClient()
{
    EpollClient::close();
    messageParsingLayer("Disconnected login link... try connect in loop");
    return true;
}

//input/ouput layer
void LinkToLogin::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    //critical error, prefer restart from 0
    abort();

    disconnectClient();
}

void LinkToLogin::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void LinkToLogin::errorParsingLayer(const char * const error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToLogin::messageParsingLayer(const char * const message) const
{
    std::cout << message << std::endl;
}

BaseClassSwitch::EpollObjectType LinkToLogin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void LinkToLogin::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool LinkToLogin::registerStatsClient(const char * const dynamicToken)
{
    if(queryNumberList.empty())
        return false;

    //send the network query
    registerOutputQuery(queryNumberList.back(),0xAD);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xAD;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    {
        SHA224 hashFile = SHA224();
        hashFile.init();
        hashFile.update(reinterpret_cast<const unsigned char *>(LinkToLogin::private_token_statclient),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        hashFile.update(reinterpret_cast<const unsigned char *>(dynamicToken),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        hashFile.final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput));
        posOutput+=CATCHCHALLENGER_SHA224HASH_SIZE;
        //memset(LinkToLogin::private_token,0x00,sizeof(LinkToLogin::private_token));->to reconnect after be disconnected
    }

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    return true;
}

void LinkToLogin::tryReconnect()
{
    stat=Stat::Unconnected;
    if(stat!=Stat::Unconnected)
        return;
    else
    {
        //std::cout << "Try reconnect to login..." << std::endl;
        resetForReconnect();
        jsonFileContent.clear();
        #ifndef STATSODROIDSHOW2
        removeJsonFile();
        #endif
        if(tryInterval<=0 || tryInterval>=60)
            this->tryInterval=5;
        if(considerDownAfterNumberOfTry<=0 || considerDownAfterNumberOfTry>=60)
            this->considerDownAfterNumberOfTry=3;
        do
        {
            stat=Stat::Connecting;
            //start to connect
            auto start = std::chrono::high_resolution_clock::now();
            connectInternal();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<(uint32_t)tryInterval*1000 && stat!=Stat::Connected)
            {
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        } while(stat!=Stat::Connected);
        readTheFirstSslHeader();
    }
}

void LinkToLogin::sendProtocolHeader()
{
    if(stat!=Stat::Connected)
    {
        std::cerr << "ERROR LinkToLogin::sendProtocolHeader() already send (abort)" << std::endl;
        abort();
    }
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xA0);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xA0;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,reinterpret_cast<char *>(LinkToLogin::header_magic_number),sizeof(LinkToLogin::header_magic_number));
    posOutput+=sizeof(LinkToLogin::header_magic_number);

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    stat=ProtocolSent;
}

void LinkToLogin::moveClientFastPath(const uint8_t &,const uint8_t &)
{
}

void LinkToLogin::updateJsonFile(const bool &withIndentation)
{
    #ifndef STATSODROIDSHOW2
    //std::cout << "Update the json file..." << std::endl;

    if(pFile==NULL && !LinkToLogin::linkToLogin->pFilePath.empty())
    {
        LinkToLogin::linkToLogin->pFile = fopen(LinkToLogin::linkToLogin->pFilePath.c_str(),"wb");
        if(LinkToLogin::linkToLogin->pFile==NULL)
        {
            std::cerr << "Unable to open the output file: " << LinkToLogin::linkToLogin->pFilePath << std::endl;
            abort();
        }
    }

    std::string content;

    std::unordered_map<uint8_t/*group index*/,std::string > serverByGroup;
    unsigned int index=0;
    while(index<serverList.size())
    {
        const ServerFromPoolForDisplay &server=serverList.at(index);
        if(server.logicalGroupIndex<logicalGroups.size())
        {
            std::string serverString;
            if(withIndentation)
                serverString+="    ";
            //serverString+="\""+std::to_string(server.groupIndex)+"-"+std::to_string(server.uniqueKey)+"\":";
            serverString+="{";
            std::string tempXml=server.xml;
            stringreplaceAll(tempXml,"/","\\/");
            stringreplaceAll(tempXml,"\"","\\\"");
            serverString+="\"xml\":\""+tempXml+"\",";
            serverString+="\"connectedPlayer\":"+std::to_string(server.currentPlayer)+",";
            if(server.maxPlayer<65534)
                serverString+="\"maxPlayer\":"+std::to_string(server.maxPlayer)+",";
            serverString+="\"charactersGroup\":"+std::to_string(server.groupIndex)+",";
            serverString+="\"uniqueKey\":"+std::to_string(server.uniqueKey)+"";
            serverString+="}";
            if(serverByGroup.find(server.logicalGroupIndex)==serverByGroup.cend())
                serverByGroup[server.logicalGroupIndex]=serverString;
            else
            {
                if(withIndentation)
                    serverByGroup[server.logicalGroupIndex]+=",\n"+serverString;
                else
                    serverByGroup[server.logicalGroupIndex]+=","+serverString;
            }
        }
        else
        {
            if(!logicalGroups.empty())
                if(!warningLogicalGroupOutOfBound)
                {
                    warningLogicalGroupOutOfBound=true;
                    std::cerr << "Server " << server.uniqueKey << "-" << server.groupIndex << " have logicalGroupIndex of of bound:"
                              << server.logicalGroupIndex << ">=" << logicalGroups.size() << std::endl;
                }
        }
        index++;
    }

    {
        unsigned int indexLogicalGroups=0;
        while(indexLogicalGroups<logicalGroups.size())
        {
            const LogicalGroup &logicalGroup=logicalGroups.at(indexLogicalGroups);
            if(serverByGroup.find(indexLogicalGroups)!=serverByGroup.end())
            {
                if(!content.empty())
                {
                    content+=",";
                    if(withIndentation)
                        content+="\n";
                }
                if(withIndentation)
                    content+="  ";
                content+="\""+logicalGroup.path+"\":{";
                if(withIndentation)
                    content+="\n";
                std::string tempXml=logicalGroup.xml;
                stringreplaceAll(tempXml,"/","\\/");
                stringreplaceAll(tempXml,"\"","\\\"");
                if(withIndentation)
                    content+="  ";
                content+="\"xml\":\""+tempXml+"\",";
                if(withIndentation)
                    content+="\n  ";
                content+="\"servers\":[";
                if(withIndentation)
                    content+="\n";
                content+=serverByGroup.at(indexLogicalGroups);
                if(withIndentation)
                    content+="\n  ";
                content+="]";
                if(withIndentation)
                    content+="\n  ";
                content+="}";
            }
            indexLogicalGroups++;
        }
    }
    if(withIndentation)
        content="\n"+content;
    content="{"+content;
    if(withIndentation)
        content=content+"\n";
    content=content+"}";
    jsonFileContent=content;

    if(pFile!=NULL)
    {
        if(fseek(pFile,0,SEEK_SET)!=0)
        {
            std::cerr << "unable to seek the output file: " << errno << std::endl;
            abort();
        }
        if(fwrite(content.data(),1,content.size(),pFile)!=content.size())
        {
            std::cerr << "unable to write the output file: " << errno << std::endl;
            abort();
        }
        if(fflush(pFile)!=0)
        {
            std::cerr << "unable to flush the output file: " << errno << std::endl;
            abort();
        }
        if(ftruncate(fileno(pFile),content.size())!=0)
        {
            std::cerr << "unable to resize the output file: " << errno << std::endl;
            abort();
        }
    }
    #endif
}

#ifndef STATSODROIDSHOW2
void LinkToLogin::removeJsonFile()
{
    if(pFile!=NULL)
    {
        std::cout << "close fd: " << pFile << __FILE__ << ":" << __LINE__ << std::endl;
        fclose(pFile);
        pFile=NULL;
    }
    if(remove(pFilePath.c_str())!=0)
    {
        if(errno!=2)
        {
            std::cerr << "Error deleting file: " << pFilePath << ", errno: " << errno << std::endl;
            abort();
        }
    }
}
#endif

#ifdef STATSODROIDSHOW2
void LinkToLogin::writeData(const char * const str)
{
    const int fd=usbdev;
    ::write(fd, "\006", 1);
    int length = strlen(str) + 48;
    ::write(fd, &length, 1);
    int isbusy=0;
    while (isbusy != '6') {
        ::read(fd, &isbusy, 1);
        usleep(10000);
    }
    ::write(fd, str, length - 48);
    isbusy = 0;
}

void LinkToLogin::writeData(const std::string &str)
{
    writeData(str.c_str());
}
#endif

ssize_t LinkToLogin::readFromSocket(char * data, const size_t &size)
{
    return EpollClient::read(data,size);
}

ssize_t LinkToLogin::writeToSocket(const char * const data, const size_t &size)
{
    if(EpollClient::write(data,size)!=(ssize_t)size)
    {
        std::cerr << "error to write ProtocolParsingInputOutput::write() " << infd << " into LinkToLogin::write(): " << binarytoHexa(data,size) << std::endl;
        return -1;
    }
    return size;
}

void LinkToLogin::closeSocket()
{
    disconnectClient();
}

