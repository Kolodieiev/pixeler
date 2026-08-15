#pragma once

#include <cstdint>

#include "RigidOffsets.h"

namespace pixeler
{
  struct SpriteGeometry
  {
    RigidOffsets rigid_offsets{};  // Опис зміщення жорсткого тіла об'єкта від країв його спрайта
    uint16_t width{1};             // Ширина спрайта
    uint16_t height{1};            // Висота спрайта
    uint16_t x_pivot{1};           // Х-координата логічного центру спрайта
    uint16_t y_pivot{1};           // Y-координата логічного центра спрайта
  };
}  // namespace pixeler
