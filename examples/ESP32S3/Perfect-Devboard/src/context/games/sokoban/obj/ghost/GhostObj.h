#pragma once

#include "game/2D/IGameObject2D.h"

using namespace pixeler;

namespace sokoban
{
  class GhostObj : public IGameObject2D
  {
  public:
    GhostObj(uint32_t id, IGameScene2D& game_scene, SfxPlayer& audio);
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
