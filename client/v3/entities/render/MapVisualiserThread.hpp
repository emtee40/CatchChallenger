#ifndef MAPVISUALISERTHREAD_H
#define MAPVISUALISERTHREAD_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QString>

#include "../../../../general/base/CommonMap.hpp"
#include "../../../../general/base/GeneralStructures.hpp"
#include "../../../../general/base/GeneralVariable.hpp"
#include "../../../../general/base/Map_loader.hpp"
#include "../../../libqtcatchchallenger/DisplayStructures.hpp"
#include "../../libraries/tiled/tiled_isometricrenderer.hpp"
#include "../../libraries/tiled/tiled_map.hpp"
#include "../../libraries/tiled/tiled_mapobject.hpp"
#include "../../libraries/tiled/tiled_mapreader.hpp"
#include "../../libraries/tiled/tiled_objectgroup.hpp"
#include "../../libraries/tiled/tiled_orthogonalrenderer.hpp"
#include "../../libraries/tiled/tiled_tile.hpp"
#include "../../libraries/tiled/tiled_tilelayer.hpp"
#include "../../libraries/tiled/tiled_tileset.hpp"
#include "../Map_client.hpp"
#include "MapDoor.hpp"
#include "MapVisualiserOrder.hpp"
#include "TriggerAnimation.hpp"

class MapVisualiserThread
#ifndef NOTHREADS
    : public QThread
#else
    : public QObject
#endif
    ,
      public MapVisualiserOrder {
  Q_OBJECT
 public:
  std::unordered_map<Tiled::Tile *, TriggerAnimationContent>
      tileToTriggerAnimationContent;

  explicit MapVisualiserThread();
  ~MapVisualiserThread();
  std::string mLastError;
  bool debugTags;
  Tiled::Tileset *tagTileset;
  int tagTilesetIndex;
  volatile bool stopIt;
  bool hideTheDoors;
  std::string error();
 signals:
  void asyncMapLoaded(const std::string &fileName, Map_full *parsedMap);
 public slots:
  void loadOtherMapAsync(const std::string &fileName);
  Map_full *loadOtherMap(const std::string &fileName);
  // drop and remplace by Map_loader info
  bool loadOtherMapClientPart(Map_full *parsedMap);
  bool loadOtherMapMetaData(Map_full *parsedMap);
  void loadBotFile(const std::string &file);
 public slots:
  virtual void resetAll();

 private:
  std::unordered_map<std::string, Map_full> mapCache;
  Tiled::MapReader reader;
  std::unordered_map<
      std::string /*name*/,
      std::unordered_map<uint32_t /*bot id*/, CatchChallenger::Bot> >
      botFiles;
  std::string language;
};

#endif  // MAPVISUALISERTHREAD_H