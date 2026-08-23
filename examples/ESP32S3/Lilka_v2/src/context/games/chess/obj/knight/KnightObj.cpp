#include "KnightObj.h"

#include "../TypeID.h"
#include "KnightMove.h"

namespace chess
{
  KnightObj::KnightObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_KNIGHT, new KnightMove())
  {
  }

  KnightObj::~KnightObj()
  {
  }
}  // namespace chess
