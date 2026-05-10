/**
 * @file Pixeler.h
 * @brief Головний клас фреймворку.
 * @details Використовується для базової ініціалізації та запуску фреймворку Pixeler з користувацького коду.
 * Не може бути реалізований в коді. Метод ініціалізації фреймворку доступний статично.
 */

#pragma once
#include "defines.h"

namespace pixeler
{
  class Pixeler
  {
  public:
    /**
     * @brief Виконує початкову ініціалізацію фреймворку та запускає задачу менеджера контекстів.
     *
     * @param stack_depth_kb Розмір стеку Pixeler в кілобайтах.
     * Пам'ять цього стеку буде використовуватися на потреби фреймворку, а також під час виконання будь-якого із контекстів.
     */
    static void begin(uint32_t stack_depth_kb);

  private:
    Pixeler();
    static void pixelerContextTask(void* params);
  };
}  // namespace pixeler
