#include "BishopObj.h"

#include "../TypeID.h"
#include "BishopMove.h"
#include "bishop_img.h"

namespace chess
{
  BishopObj::BishopObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_BISHOP, BISHOP_SPRITES, new BishopMove())
  {
  }

  BishopObj::~BishopObj()
  {
  }
}  // namespace chess
