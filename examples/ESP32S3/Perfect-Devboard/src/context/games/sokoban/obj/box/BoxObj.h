#pragma once

#include "game/IGameObject.h"

using namespace pixeler;

namespace sokoban
{
  class BoxObj : public IGameObject
  {
  public:
    BoxObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~BoxObj() {}
    virtual void __update() override;
    virtual void serialize(DataStream& ds) override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;

    // Якщо можливо, переміститися в задані координати. Повертає true в разі успіху
    bool moveTo(uint16_t x, uint16_t y);

    inline bool isOk() const
    {
      return _is_ok;
    }

  private:
    bool _is_ok{false};  // Вказує, чи встановлено ящик в ключову точку
  };
}  // namespace sokoban
