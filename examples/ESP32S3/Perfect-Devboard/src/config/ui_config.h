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
#include "context/files/FilesContext.h"
#include "context/firmware/FirmwareContext.h"
#include "context/games/GameListContext.h"
#include "context/home/HomeContext.h"
#include "context/menu/MenuContext.h"
#include "context/mp3/Mp3Context.h"
#include "context/pref/PrefSelectContext.h"
#include "context/reader/ReaderContext.h"
#include "context/splash/SplashContext.h"
#include "context/wifi/WiFiContext.h"
// Ігрові контексти
#include "context/games/sokoban/SokobanContext.h"
#include "context/games/chess/ChessContext.h"

namespace pixeler
{
  // -------------------------------- Додай перемикання контексту за прикладом

  /**
   * @brief Фабрика генерації об'єкту контексту по його ідентифікатору.
   *
   */
  const std::unordered_map<ContextID, std::function<IContext*()>> _context_id_map = {
      {ContextID::ID_CONTEXT_SPLASH, []()
       { return new SplashContext(); }},
      {ContextID::ID_CONTEXT_HOME, []()
       { return new HomeContext(); }},
      {ContextID::ID_CONTEXT_MENU, []()
       { return new MenuContext(); }},
      {ContextID::ID_CONTEXT_MP3, []()
       { return new Mp3Context(); }},
      {ContextID::ID_CONTEXT_FILES, []()
       { return new FilesContext(); }},
      {ContextID::ID_CONTEXT_GAMES, []()
       { return new GameListContext(); }},
      {ContextID::ID_CONTEXT_PREF_SEL, []()
       { return new PrefSelectContext(); }},
      {ContextID::ID_CONTEXT_READER, []()
       { return new ReaderContext(); }},
      {ContextID::ID_CONTEXT_FIRMWARE, []()
       { return new FirmwareContext(); }},
      {ContextID::ID_CONTEXT_WIFI, []()
       { return new WiFiContext(); }},
      {ContextID::ID_CONTEXT_SOKOBAN, []()
       { return new sokoban::SokobanContext(); }},
      {ContextID::ID_CONTEXT_CHESS, []()
       { return new chess::ChessContext(); }},
  };
}  // namespace pixeler

// -------------------------------- Стартовий контекст
#define START_CONTEXT SplashContext  // Клас контексту, який буде завантажено самим першим, після ініціалізації Pixeler.
