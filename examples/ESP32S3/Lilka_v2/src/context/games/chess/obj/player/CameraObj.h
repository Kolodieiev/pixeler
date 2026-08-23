#pragma once

#include "game/2D/IGameObject2D.h"

using namespace pixeler;

namespace chess
{
  class CameraObj : public IGameObject2D
  {
  public:
    CameraObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio);
    virtual ~CameraObj();

    virtual void __update() override;
    virtual void serialize(DataStream& ds) const override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;

  private:
  };
}  // namespace chess
