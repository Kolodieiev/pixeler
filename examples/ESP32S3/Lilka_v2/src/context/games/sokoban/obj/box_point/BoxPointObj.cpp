#include "BoxPointObj.h"

#include "../TypeID.h"

namespace sokoban
{
  BoxPointObj::BoxPointObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IGameObject(id, TYPE_BOX_DOCK, game_scene, audio)
  {
    _sprite.has_img = true;
    setImgVariant(0);
    setGeometryVariant(0);
  }

  void BoxPointObj::__update()
  {
  }

  void BoxPointObj::serialize(DataStream& ds) const
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
