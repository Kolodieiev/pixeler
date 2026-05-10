#pragma once
#pragma GCC optimize("O3")
#include "lib/lua/lua.h"

/**
 * @brief Перевіряє, чи належить об'єкт з вказаною позицією на стеку до вказаного типу.
 * Викликає luaL_error, якщо тип не співпадає.
 *
 * @param L Lua
 * @param index Позиція об'єкта на стеку.
 * @param type_name Ім'я типу.
 * @return void* - Якщо об'єкт належить до типу. Інакше - nullptr.
 */
void* lua_check_instance(lua_State* L, int index, const char* type_name);