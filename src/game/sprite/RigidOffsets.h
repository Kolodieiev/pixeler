#pragma once

#include <cstdint>

namespace pixeler
{
  /**
   * @brief Структура для опису зміщення жорсткого тіла об'єкта від країв його спрайта
   *
   */
  struct RigidOffsets
  {
    uint16_t top{0};     // Зміщення від верхнього краю.
    uint16_t bottom{0};  // Зміщення від нижнього краю.
    uint16_t left{0};    // Зміщення від лівого краю.
    uint16_t right{0};   // Зміщення від правого краю.
  };
}  // namespace pixeler
