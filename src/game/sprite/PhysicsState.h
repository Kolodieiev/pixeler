#pragma once

#include <cstdint>

namespace pixeler
{
  /**
   * @brief Структура для опису стану фізичних властивостей ігрового об'єкта
   *
   */
  struct PhysicsState
  {
    uint16_t pass_ability_mask{0};  // Маска, що задає тип прохідності ігровго об'єкта по плитках ігрового рівня
    bool is_rigid{false};           // Чи має об'єкт тверде тіло
  };
}  // namespace pixeler
