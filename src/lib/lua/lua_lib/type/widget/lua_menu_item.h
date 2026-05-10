#pragma GCC optimize("O3")
#pragma once
#include "lib/lua/lua.h"

const char STR_TYPE_NAME_MENU_ITEM[] = "MenuItem";

void lua_init_menu_item(lua_State* L);
