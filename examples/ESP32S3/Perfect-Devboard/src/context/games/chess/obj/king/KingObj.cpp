#include "KingObj.h"

#include "../TypeID.h"
#include "KingMove.h"
#include "king_img.h"

namespace chess
{
  KingObj::KingObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_KING, KING_SPRITES, new KingMove())
  {
  }

  KingObj::~KingObj()
  {
  }
}  // namespace chess
