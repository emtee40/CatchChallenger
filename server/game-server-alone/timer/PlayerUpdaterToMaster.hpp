#ifndef CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H
#define CATCHCHALLENGER_PLAYERUPDATERTOMASTER_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../epoll/EpollTimer.hpp"

namespace CatchChallenger {
class PlayerUpdaterToMaster
        : public EpollTimer
{
public:
    static PlayerUpdaterToMaster player_updater_to_master;
public:
    explicit PlayerUpdaterToMaster();
private:
    void exec();
private:
    unsigned short int sended_connected_players;
    enum FrequencyUpdate
    {
        FrequencyUpdate_slow,
        FrequencyUpdate_medium,
        FrequencyUpdate_fast,
    };
    FrequencyUpdate frequencyUpdate;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // PLAYERUPDATERTOMASTER_H
