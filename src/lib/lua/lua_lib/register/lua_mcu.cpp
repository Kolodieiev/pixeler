#pragma GCC optimize("O3")
#include "lua_mcu.h"

#include <stdint.h>

const char STR_LIB_NAME_MCU[] = "mcu";
//

int lua_mcu_millis(lua_State* L)
{
  lua_pushinteger(L, millis());
  return 1;
}

int lua_mcu_delay(lua_State* L)
{
  int ms = luaL_checkinteger(L, 1);
  delay(ms);
  return 0;
}

int lua_mcu_set_cpu_freq(lua_State* L)
{
  int freq = luaL_checkinteger(L, 1);
  lua_pushboolean(L, setCpuFrequencyMhz(freq));
  return 1;
}

//----------------------------------------------------------------------------------------------------

const struct luaL_Reg LIB_MCU[] = {
    {"millis", lua_mcu_millis},
    {"delay", lua_mcu_delay},
    {"setCpuFreq", lua_mcu_set_cpu_freq},
    {nullptr, nullptr},
};

int lua_register_mcu(lua_State* L)
{
  luaL_newlib(L, LIB_MCU);
  lua_setglobal(L, STR_LIB_NAME_MCU);
  return 0;
}