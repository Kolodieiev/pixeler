#include "IPiece.h"

#include "Board.h"
#include "IMoveStrategy.h"

namespace chess
{
  static const uint8_t VARIANT_BLACK = 0;
  static const uint8_t VARIANT_WHITE = 1;

  IPiece::IPiece(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio, uint16_t type_id, IMoveStrategy* movement)
      : IGameObject2D(id, type_id, game_scene, audio), _movement{movement}
  {
    _sprite.has_img = true;
    setImgVariant(VARIANT_WHITE);
    setGeometryVariant(0);
  }

  IPiece::~IPiece()
  {
    delete _movement;
  }

  std::vector<Position> IPiece::getPossibleMoves(const Board& board) const
  {
    return _movement->calcMoves(_x_global, _y_global, board);
  }

  void IPiece::__update()
  {
  }

  void IPiece::serialize(DataStream& ds) const
  {
  }

  void IPiece::deserialize(DataStream& ds)
  {
  }

  size_t IPiece::getDataSize() const
  {
    return 0;
  }

  void IPiece::setIsWhite(bool state)
  {
    _is_white = state;

    if (_is_white)
      setImgVariant(VARIANT_WHITE);
    else
      setImgVariant(VARIANT_BLACK);
  }

  bool IPiece::isWhite() const
  {
    return _is_white;
  }

  void IPiece::destroy()
  {
    _is_alive = false;
  }

  void IPiece::rotateSprite(int16_t angle)
  {
    _sprite.angle = angle;
  }
}  // namespace chess
