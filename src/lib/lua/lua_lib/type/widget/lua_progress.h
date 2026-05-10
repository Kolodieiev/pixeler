#pragma once
#pragma GCC optimize("O3")
#include "lib/lua/lua.h"

const char STR_TYPE_NAME_PROGRESS[] = "ProgressBar";

void lua_init_progress(lua_State* L);