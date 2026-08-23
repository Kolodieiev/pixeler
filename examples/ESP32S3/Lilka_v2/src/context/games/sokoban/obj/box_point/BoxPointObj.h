#pragma once

#include "game/2D/IGameObject2D.h"

using namespace pixeler;

namespace sokoban
{
  class BoxPointObj : public IGameObject2D
  {
  public:
    BoxPointObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio);
    virtual ~BoxPointObj() {}

    virtual void __update() override;
    virtual void serialize(DataStream& ds) const override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;
  };
}  // namespace sokoban
