#pragma once

#include <cstdint>

#include "SpriteGeometry.h"

namespace pixeler
{
  /**
   * @brief Структура для опису стану спрайта ігровго об'єкта
   *
   */
  struct SpriteState
  {
    const std::vector<const uint16_t*>* animation{nullptr};  // Вказівник на вектор поточної анімації
    const uint16_t* image{nullptr};                          // Вказівник на массив із даними зображення, якщо відсутня анімація
    int16_t angle{0};                                        // Кут повороту спрайта
    uint8_t frames_counter{0};                               // Лічильник кадрів
    uint8_t anim_pos{0};                                     // Номер поточного кадру анімації
    uint8_t frames_between_anim{0};                          // Кількість пропущених кадрів між кадрами анімації
    bool has_img{false};                                     // Чи має об'єкт статичне зображення спрайта
    bool has_animation{false};                               // Чи має об'єкт анімацію спрайта

    /**
     * @brief Скидає лічильники кадрів та позиції поточної анімації ігрового об'єкта.
     *
     */
    void resetAnimation()
    {
      frames_counter = 0;
      anim_pos = 0;
    }
  };
}  // namespace pixeler
