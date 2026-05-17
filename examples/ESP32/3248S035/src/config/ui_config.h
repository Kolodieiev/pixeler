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

#include "driver/graphics/DisplayWrapper.h"
#include "context_id_config.h"

// -------------------------------- Підключи нижче заголовкові файли контекстів першого рівня
#include "context/files/FilesContext.h"
#include "context/home/HomeContext.h"
#include "context/menu/MenuContext.h"
#include "context/splash/SplashContext.h"
#include "context/wifi/WiFiContext.h"

namespace pixeler
{
  // -------------------------------- Додай перемикання контексту за прикладом

  /**
   * @brief Фабрика генерації об'єкту контексту по його ідентифікатору.
   *
   */
  std::unordered_map<ContextID, std::function<IContext*()>> _context_id_map = {
      {ContextID::ID_CONTEXT_SPLASH, []()
       { return new SplashContext(); }},
      {ContextID::ID_CONTEXT_HOME, []()
       { return new HomeContext(); }},
      {ContextID::ID_CONTEXT_MENU, []()
       { return new MenuContext(); }},
      {ContextID::ID_CONTEXT_FILES, []()
       { return new FilesContext(); }},
      {ContextID::ID_CONTEXT_WIFI, []()
       { return new WiFiContext(); }},
  };
}  // namespace pixeler

// -------------------------------- Стартовий контекст
#define START_CONTEXT SplashContext  // Клас контексту, який буде завантажено самим першим, після ініціалізації Pixeler.
