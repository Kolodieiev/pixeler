#include "lua_widget_helper.h"

#include "lib/lua/lua.h"

bool lua_is_instance(lua_State* L, int index, const char* type_name)
{
  if (!lua_isuserdata(L, index))
    return false;

  if (luaL_testudata(L, index, type_name))
    return true;

  if (!lua_getmetatable(L, index))
    return false;

  luaL_getmetatable(L, type_name);

  int target_idx = lua_gettop(L);    // Індекс цільової метатаблиці
  int current_idx = target_idx - 1;  // Індекс поточної метатаблиці

  bool found = false;
  for (int i = 0; i < 10; ++i)
  {
    // Отримуємо __index
    lua_getfield(L, current_idx, "__index");

    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      break;
    }

    // Отримуємо метатаблицю __index (таблиці методів)
    if (lua_getmetatable(L, -1))
    {
      // Порівнюємо з цільовою метатаблицею
      if (lua_rawequal(L, -1, target_idx))
      {
        found = true;
        lua_pop(L, 2);  // Видаляємо метатаблицю __index та сам __index
        break;
      }

      // Видаляємо __index таблицю, залишаємо її метатаблицю
      lua_remove(L, -2);

      // Замінюємо поточну метатаблицю
      lua_replace(L, current_idx);
    }
    else
    {
      lua_pop(L, 1);
      break;
    }
  }

  lua_pop(L, 2);  // Очищаємо обидві метатаблиці
  return found;
}

void* lua_check_instance(lua_State* L, int index, const char* type_name)
{
  if (!lua_is_instance(L, index, type_name))
  {
    luaL_error(L, "Очікувався об'єкт типу '%s' або його нащадок", type_name);
    return nullptr;
  }

  return lua_touserdata(L, index);
}