#pragma once

#include "game/IGameObject.h"

using namespace pixeler;

namespace chess
{
  class CameraObj : public IGameObject
  {
  public:
    CameraObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~CameraObj();

    virtual void __update() override;
    virtual void serialize(DataStream& ds) const override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;

  private:
  };
}  // namespace chess
