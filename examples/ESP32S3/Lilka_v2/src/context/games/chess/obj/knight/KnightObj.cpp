#include "KnightObj.h"

#include "../TypeID.h"
#include "KnightMove.h"
#include "knight_img.h"

namespace chess
{
  KnightObj::KnightObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_KNIGHT, KNIGHT_SPRITES, new KnightMove())
  {
  }

  KnightObj::~KnightObj()
  {
  }
}  // namespace chess
