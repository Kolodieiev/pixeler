#include "BoxPointObj.h"

#include "../TypeID.h"
#include "./res/sprite_box_point.h"

namespace sokoban
{
  BoxPointObj::BoxPointObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IGameObject(id, TYPE_BOX_POINT, game_scene, audio)
  {
    _sprite.img_ptr = SPRITE_BOX_POIN;
    _sprite.has_img = true;
    _sprite.width = 32;
    _sprite.height = 32;

    _sprite.is_rigid = false;
  }

  void BoxPointObj::__update()
  {
  }

  void BoxPointObj::serialize(DataStream& ds)
  {
  }

  void BoxPointObj::deserialize(DataStream& ds)
  {
  }

  size_t BoxPointObj::getDataSize() const
  {
    return 0;
  }
}  // namespace sokoban
