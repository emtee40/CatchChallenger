#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

LoginLinkToMaster::LoginLinkToMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        ) :
        ProtocolParsingInputOutput(
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ,PacketModeTransmission_Client
            #endif
            ),
        have_send_protocol(false),
        is_logging_in_progess(false)
{
    int index=0;
    while(index<10)
    {
        queryNumberList << index;
        index++;
    }
}

LoginLinkToMaster::~LoginLinkToMaster()
{
    closeSocket();
}

int LoginLinkToMaster::tryConnect(const char * const host, const quint16 &port,const quint8 &tryInterval,const quint8 &considerDownAfterNumberOfTry)
{
    int sockfd,n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0)
    {
        std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
        abort();
    }
    server=gethostbyname(host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to master server (abort)" << std::endl;
        abort();
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    std::cout << "Try connect to master..." << std::endl;
    int connStatusType=::connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType<0)
        {
            sleep(tryInterval);
            connStatusType=::connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
        }
        if(connStatusType<0)
        {
            std::cerr << "ERROR connecting to master server (abort)" << std::endl;
            abort();
        }
    }
    std::cout << "Connected to master" << std::endl;
    char buffer[1];
    n=::read(sockfd,buffer,1);
    if(n<0)
    {
        std::cerr << "ERROR reading from socket to master server (abort)" << std::endl;
        abort();
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
    return sockfd;
}

void LoginLinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LoginLinkToMaster::errorParsingLayer(const QString &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const QString &message) const
{
    std::cout << socketString << ": " << message.toLocal8Bit().constData() << std::endl;
}

void LoginLinkToMaster::errorParsingLayer(const char * const error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::Type LoginLinkToMaster::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void LoginLinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}