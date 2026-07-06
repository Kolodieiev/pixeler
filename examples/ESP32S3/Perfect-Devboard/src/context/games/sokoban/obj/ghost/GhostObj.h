#pragma once

#include "game/IGameObject.h"

using namespace pixeler;

namespace sokoban
{
  class GhostObj : public IGameObject
  {
  public:
    GhostObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~GhostObj() {}

    virtual void __update() override;
    virtual void serialize(DataStream& ds) const override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;

    void move(MovingDirection direction);

  private:
    const uint16_t PIX_PER_STEP{20};  // Кількість пікселів пройдених за кадр
  };
}  // namespace sokoban
