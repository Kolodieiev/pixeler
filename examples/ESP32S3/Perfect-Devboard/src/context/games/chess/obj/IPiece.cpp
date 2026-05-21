#include "IPiece.h"

#include "Board.h"
#include "IMoveStrategy.h"

namespace chess
{
  IPiece::IPiece(uint32_t id, IGameScene& game_scene, SfxPlayer& audio, uint16_t type_id, const unsigned short* const sprite_arr[], IMoveStrategy* movement)
      : IGameObject(id, type_id, game_scene, audio), _SPRITE_ARR{sprite_arr}, _movement{movement}
  {
    _sprite.has_img = true;
    _sprite.img_ptr = _SPRITE_ARR[1];  // За замовченням білі
    _sprite.width = 26;
    _sprite.height = 26;
    _sprite.x_pivot = _sprite.width / 2;
    _sprite.y_pivot = _sprite.height / 2;
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

  void IPiece::serialize(DataStream& ds)
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
      _sprite.img_ptr = _SPRITE_ARR[1];
    else
      _sprite.img_ptr = _SPRITE_ARR[0];
  }

  bool IPiece::isWhite() const
  {
    return _is_white;
  }

  void IPiece::destroy()
  {
    _is_destroyed = true;
  }

  void IPiece::rotateSprite(int16_t angle)
  {
    _sprite.angle = angle;
  }
}  // namespace chess
