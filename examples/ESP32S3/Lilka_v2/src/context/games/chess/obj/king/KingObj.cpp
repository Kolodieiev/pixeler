#include "KingObj.h"

#include "../TypeID.h"
#include "KingMove.h"

namespace chess
{
  KingObj::KingObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_KING, new KingMove())
  {
  }

  KingObj::~KingObj()
  {
  }
}  // namespace chess
