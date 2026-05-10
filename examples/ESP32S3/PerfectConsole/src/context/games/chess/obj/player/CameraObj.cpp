#include "CameraObj.h"

#include "../TypeID.h"

namespace chess
{
  CameraObj::CameraObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IGameObject(id, TYPE_PLAYER, game_scene, audio)
  {
    _sprite.is_rigid = false;
  }

  CameraObj::~CameraObj()
  {
  }

  void CameraObj::__update()
  {
  }

  void CameraObj::serialize(DataStream& ds)
  {
  }

  void CameraObj::deserialize(DataStream& ds)
  {
  }

  size_t CameraObj::getDataSize() const
  {
    return 0;
  }
}  // namespace chess
