#include "QueenObj.h"

#include "../TypeID.h"
#include "QueenMove.h"
#include "queen_img.h"

namespace chess
{
  QueenObj::QueenObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_QUEEN, QUEEN_SPRITES, new QueenMove())
  {
  }

  QueenObj::~QueenObj()
  {
  }
}  // namespace chess
