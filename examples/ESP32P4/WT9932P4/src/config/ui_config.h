/**
 * @file ui_config.h
 * @brief Файл підключення класів контексту
 * @details В даний файл необхідно підключити заголовкові файли класів контексту та додати функцію їх створення в map.
 * Файли контексту необхідно підключити власні за прикладом нижче.
 *
 * Ідентифікатори контексту(ContextID) додаються у файлі "pixeler/setup/context_id_config.h"
 */

#pragma once
#include <stdint.h>

#include <functional>
#include <unordered_map>

#include "context_id_config.h"
#include "driver/graphics/DisplayWrapper.h"

// -------------------------------- Підключи нижче заголовкові файли контекстів першого рівня
#include "context/home/HomeContext.h"

namespace pixeler
{
  // -------------------------------- Додай перемикання контексту за прикладом

  /**
   * @brief Фабрика генерації об'єкту контексту по його ідентифікатору.
   *
   */
  std::unordered_map<ContextID, std::function<IContext*()>> _context_id_map = {
      {ContextID::ID_CONTEXT_HOME, []()
       { return new HomeContext(); }}
  };
}  // namespace pixeler

// -------------------------------- Стартовий контекст
#define START_CONTEXT HomeContext  // Клас контексту, який буде завантажено самим першим, після ініціалізації Pixeler.
