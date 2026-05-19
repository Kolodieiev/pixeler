/**
 * @file ui_config.h
 * @brief Файл підключення класів контексту
 * @details В даний файл необхідно підключити заголовкові файли класів контексту та додати функцію їх створення в map.
 * Файли контексту необхідно підключити власні за прикладом нижче.
 *
 * Ідентифікатори контексту(ContextID) додаються у файлі "context_id_config.h"
 */

#pragma once
#include <stdint.h>

#include <functional>
#include <unordered_map>

#include "context_id_config.h"
#include "driver/graphics/DisplayWrapper.h"

// -------------------------------- Підключи нижче заголовкові файли контекстів першого рівня
#include "context/menu/MenuContext.h"
#include "context/wifi/WiFiContext.h"

namespace pixeler
{
  // -------------------------------- Додай перемикання контекста за прикладом

  /**
   * @brief Фабрика генерації об'єкта контекста по його ідентифікатору.
   *
   */
  std::unordered_map<ContextID, std::function<IContext*()>> _context_id_map = {
      {ContextID::ID_CONTEXT_MENU, []()
       { return new MenuContext(); }},
      {ContextID::ID_CONTEXT_WIFI, []()
       { return new WiFiContext(); }},
  };
}  // namespace pixeler

// -------------------------------- Стартовий контекст
#define START_CONTEXT MenuContext  // Клас контексту, який буде завантажено самим першим, після ініціалізації Pixeler.
