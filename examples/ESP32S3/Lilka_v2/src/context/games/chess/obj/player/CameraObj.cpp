#include "CameraObj.h"

#include "../TypeID.h"

namespace chess
{
  CameraObj::CameraObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio) : IGameObject2D(id, TYPE_PLAYER, game_scene, audio)
  {
  }

  CameraObj::~CameraObj()
  {
  }

  void CameraObj::__update()
  {
  }

  void CameraObj::serialize(DataStream& ds) const
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
