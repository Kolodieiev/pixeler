#pragma GCC optimize("O3")
#include "lua_pwm.h"

#include <stdint.h>

#include "esp32-hal-ledc.h"
#include "lib/lua/lua_lib/helper/lua_helper.h"

//

int lua_pwm_stop(lua_State* L)
{
  int pin = luaL_checkinteger(L, 1);
  ledcWrite(pin, 0);
  return 0;
}

int lua_pwm_attach(lua_State* L)
{
  int pin = luaL_checkinteger(L, 1);
  int freq = luaL_checkinteger(L, 2);
  int res = luaL_checkinteger(L, 3);
  ledcAttach(pin, freq, res);
  return 0;
}

int lua_pwm_detach(lua_State* L)
{
  int pin = luaL_checkinteger(L, 1);
  ledcDetach(pin);
  return 0;
}

int lua_pwm_set(lua_State* L)
{
  int pin = luaL_checkinteger(L, 1);
  int duty = luaL_checkinteger(L, 2);
  ledcWrite(pin, duty);
  return 0;
}

//----------------------------------------------------------------------------------------------------

const struct luaL_Reg LIB_PWM[] = {
    {"stop", lua_pwm_stop},
    {"attach", lua_pwm_attach},
    {"detach", lua_pwm_detach},
    {"set", lua_pwm_set},
    {nullptr, nullptr},
};

int lua_open_pwm(lua_State* L)
{
  luaL_newlib(L, LIB_PWM);
  return 1;
}