#include "RookObj.h"

#include "../TypeID.h"
#include "RookMove.h"
#include "rook_img.h"

namespace chess
{
  RookObj::RookObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_ROOK, ROOK_SPRITES, new RookMove())
  {
  }

  RookObj::~RookObj()
  {
  }
}  // namespace chess
