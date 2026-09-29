#pragma once
#include "defines.h"

namespace pixeler
{
  /**
   * @brief Зчитує значення напруги на піні.
   *
   * @param pin Пін, на якому потрібно зчитати напругу
   * @param samples_num Кількість вибірок
   * @param r_div_k Коефіцієнт подільника напруги за формулою (R2 / (R1 + R2)), якщо потрібно
   * @return float
   */
  float readPinVoltage(uint8_t pin, uint8_t samples_num = 1, float r_div_k = 1.0f);
}  // namespace pixeler
