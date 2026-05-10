#include "PawnObj.h"

#include "../TypeID.h"
#include "../queen/QueenMove.h"
#include "../queen/queen_img.h"
#include "PawnMove.h"
#include "pawn_img.h"

namespace chess
{
  PawnObj::PawnObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_PAWN, PAWN_SPRITES, new PawnMove())
  {
  }

  PawnObj::~PawnObj()
  {
  }

  void PawnObj::turnIntoQueen()
  {
    _is_queen = true;
    
    delete _movement;
    _movement = new QueenMove();

    _SPRITE_ARR = QUEEN_SPRITES;
    setIsWhite(isWhite());
  }

  bool PawnObj::isQueen() const
  {
    return _is_queen;
  }
}  // namespace chess
