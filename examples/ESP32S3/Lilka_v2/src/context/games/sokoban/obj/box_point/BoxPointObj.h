#pragma once

#include "game/IGameObject.h"

using namespace pixeler;

namespace sokoban
{
  class BoxPointObj : public IGameObject
  {
  public:
    BoxPointObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~BoxPointObj() {}

    virtual void __update() override;
    virtual void serialize(DataStream& ds) override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;
  };
}  // namespace sokoban
