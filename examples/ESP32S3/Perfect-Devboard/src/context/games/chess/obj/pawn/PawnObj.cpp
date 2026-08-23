#include "PawnObj.h"

#include "../TypeID.h"
#include "../queen/QueenMove.h"
#include "PawnMove.h"

namespace chess
{
  static const uint8_t VARIANT_QUEEN_BLACK = 2;
  static const uint8_t VARIANT_QUEEN_WHITE = 3;

  PawnObj::PawnObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio) : IPiece(id, game_scene, audio, TYPE_PAWN, new PawnMove())
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

    if (isWhite())
      setImgVariant(VARIANT_QUEEN_WHITE);
    else
      setImgVariant(VARIANT_QUEEN_BLACK);
  }

  bool PawnObj::isQueen() const
  {
    return _is_queen;
  }
}  // namespace chess
