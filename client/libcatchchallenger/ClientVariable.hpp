#ifndef CATCHCHALLENGER_CLIENTVARIABLE_H
#define CATCHCHALLENGER_CLIENTVARIABLE_H

//#define FAKESOCKETDEBUG
//#define DEBUG_BASEWINDOWS
//#define DEBUG_RENDER_CACHE
//#define DEBUG_RENDER_DISPLAY_INDIVIDUAL_LAYER
//#define DEBUG_CLIENT_LOAD_ORDER
//#define DEBUG_CLIENT_PLAYER_ON_MAP
//#define DEBUG_CLIENT_QUEST
//#define DEBUG_CLIENT_OTHER_PLAYER_MOVE_STEP
//#define DEBUG_CLIENT_BATTLE
//#define DEBUG_CLIENT_BOT
//#define DEBUG_CLIENT_NETWORK_USAGE

#define CATCHCHALLENGER_SERVER_LIST_URL "https://catchchallenger.herman-brule.com/server_list.xml"
#define CATCHCHALLENGER_UPDATER_URL "https://cdn.confiared.com/catchchallenger.herman-brule.com/updater.txt"
#define CATCHCHALLENGER_RSS_URL "https://cdn.confiared.com/catchchallenger.herman-brule.com/rss_global.xml"

#define CATCHCHALLENGER_CLIENT_MUSIC_LOADING "/music/loading.opus"
#define CATCHCHALLENGER_CLIENT_INSTANT_SHOP

#define TIMETODISPLAY_TIP 8000
#define TIMETODISPLAY_GAIN 8000
#define TIMEINMSTOBESLOW 700 /* 500ms => 100ms due to tcp cork on client, 100ms due to tcp cork on server, 300ms of internet latency */
#define TIMETOSHOWTHETURTLE 15*1000

#define CATCHCHALLENGER_CLIENT_MAP_CACHE_TIMEOUT 15*60
#define CATCHCHALLENGER_CLIENT_MAP_CACHE_SIZE 70

#define CLIENT_BASE_TILE_SIZE 16

#endif // CATCHCHALLENGER_CLIENTVARIABLE_H
