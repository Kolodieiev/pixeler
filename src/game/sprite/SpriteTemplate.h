#pragma once

#include <cstdint>
#include <vector>

#include "SpriteGeometry.h"

namespace pixeler
{
  /**
   * @brief Структура для опису спрайта ігрового об'єкта
   *
   */
  struct SpriteTemplate
  {
    std::vector<std::vector<const uint16_t*>> animation_variants;  // Всі можливі варіанти анімацій для ігрового об'єкта
    std::vector<const uint16_t*> img_variants;                     // Всі можливі варіанти зображення спрайта ігрового об'єкта
    std::vector<SpriteGeometry> geometry_variants;                 // Всі можливі варіанти геометрії спрайта ігрового об'єкта
  };
}  // namespace pixeler
