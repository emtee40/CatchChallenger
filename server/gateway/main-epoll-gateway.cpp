#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <signal.h>

#include "../../general/base/FacilityLibGeneral.hpp"
#include "../epoll/Epoll.hpp"
#include "EpollServerLoginSlave.hpp"
#include "EpollClientLoginSlave.hpp"
#include "TimerDdos.hpp"

#define MAXEVENTS 512
#define MAXCLIENTSINSUSPEND 16

#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
extern "C" {
const char* __asan_default_options() { return "alloc_dealloc_mismatch=0:detect_container_overflow=0"; }
}  // extern "C"
#  endif
#endif

using namespace CatchChallenger;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

int main(int argc, char *argv[])
{
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    LinkToGameServer::mDatapackBase=FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/";

    (void)argc;
    (void)argv;

    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::LoginServer);
    if(!Epoll::epoll.init())
        return EPOLLERR;

    //blocking due to db connexion
    EpollServerLoginSlave::epollServerLoginSlave=new EpollServerLoginSlave();

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

    TimerDdos timerDdos;
    {
        if(!timerDdos.start(CATCHCHALLENGER_DDOS_COMPUTERINTERVAL*1000))
        {
            std::cerr << "timerDdos fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }

    int numberOfConnectedClient=0;
    /* The event loop */
    std::vector<void *> elementsToDelete[16];
    size_t elementsToDeleteSize=0;
    uint8_t elementsToDeleteIndex=0;

    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        if(elementsToDeleteSize>0 && number_of_events<MAXEVENTS)
        {
            if(elementsToDeleteIndex>=15)
                elementsToDeleteIndex=0;
            else
                ++elementsToDeleteIndex;
            const std::vector<void *> &elementsToDeleteSub=elementsToDelete[elementsToDeleteIndex];
            if(!elementsToDeleteSub.empty())
            {
                unsigned int index=0;
                while(index<elementsToDeleteSub.size())
                {
                    delete static_cast<EpollClientLoginSlave *>(elementsToDeleteSub.at(index));
                    index++;
                }
            }
            elementsToDeleteSize-=elementsToDeleteSub.size();
            elementsToDelete[elementsToDeleteIndex].clear();
        }
        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::EpollObjectType::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = EpollServerLoginSlave::epollServerLoginSlave->accept(&in_addr, &in_len);
                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }
                        /* do at the protocol negociation to send the reason
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            ::close(infd);
                            break;
                        }*/

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        /*int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else*/
                        {
                            if(EpollServerLoginSlave::epollServerLoginSlave->tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(EpollServerLoginSlave::epollServerLoginSlave->tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            EpollClientLoginSlave *client=new EpollClientLoginSlave(infd
                                               #ifdef SERVERSSL
                                               ,EpollServerLoginSlave::epollServerLoginSlave->getCtx()
                                               #endif
                                );
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(static_cast<BaseClassSwitch *>(client)->getType()!=BaseClassSwitch::EpollObjectType::Client)
                            {
                                std::cerr << "Wrong post check type (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s == 0)
                                {
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                                    client->socketStringSize=strlen(hbuf)+strlen(sbuf)+1+1;
                                    client->socketString=new char[client->socketStringSize];
                                    strcpy(client->socketString,hbuf);
                                    strcat(client->socketString,":");
                                    strcat(client->socketString,sbuf);
                                    client->socketString[client->socketStringSize-1]='\0';
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLOUT;
                            const int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            else
                            {
                                const ssize_t &returnVal=::write(infd,encodingBuff,sizeof(encodingBuff));
                                if(returnVal!=sizeof(encodingBuff))
                                {
                                    std::cerr << "epoll_ctl on socket error, unable to write encodingBuff" << std::endl;
                                    delete client;
                                }
                            }
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    EpollClientLoginSlave * const client=static_cast<EpollClientLoginSlave *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;

                        client->disconnectClient();
                        elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);

                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || client->socketIsClosed())
                    {
                        numberOfConnectedClient--;

                        if(client->disconnectClient())
                            elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);
                    }
                }
                break;
                case BaseClassSwitch::EpollObjectType::ClientServer:
                {
                    LinkToGameServer * const client=static_cast<LinkToGameServer *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;

                        client->disconnectClient();
                        elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);

                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        std::cout << "event EPOLLIN " << client << std::endl;
                        client->parseIncommingData();
                    }
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || client->socketIsClosed())
                    {
                        std::cout << "event EPOLLHUP " << client << std::endl;
                        numberOfConnectedClient--;

                        if(client->disconnectClient())
                            elementsToDelete[elementsToDeleteIndex].push_back(events[i].data.ptr);
                    }
                }
                break;
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                    static_cast<EpollTimer *>(events[i].data.ptr)->validateTheTimer();
                }
                break;
                default:
                    std::cerr << "unknown event: " << static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType() << std::endl;
                break;
            }
        }
    }
    EpollServerLoginSlave::epollServerLoginSlave->close();
    delete EpollServerLoginSlave::epollServerLoginSlave;
    EpollServerLoginSlave::epollServerLoginSlave=NULL;
    return EXIT_SUCCESS;
}
