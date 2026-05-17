/**
 * @file context_id_config.h
 * @brief Файл для додавання нових ідентифікаторів контексту в проект
 * @details Використовується для збереження ідентифікаторів контексту в одному місці.
 * Допомогає підтримувати унікальність ідентифікаторів контексту для всього проекту.
 */

#pragma once
#include <stdint.h>

// -------------------------------- Додай ID контекстів
namespace pixeler
{
  enum ContextID : uint8_t
  {
    ID_CONTEXT_SPLASH = 0,
    ID_CONTEXT_HOME,
    ID_CONTEXT_MENU,
    ID_CONTEXT_TORCH,
    ID_CONTEXT_GAMES,
    ID_CONTEXT_WIFI,
    ID_CONTEXT_LUA,
    ID_CONTEXT_MP3,
    ID_CONTEXT_FILES,
    ID_CONTEXT_FIRMWARE,
    ID_CONTEXT_READER,
    ID_CONTEXT_PREF_SEL,
  };
}
