#pragma GCC optimize("O3")
#include "lua_wifi.h"

#include "../helper/lua_helper.h"
#include "manager/WiFiManager.h"
#include "lib/lua/res/lua_strs.h"

int lua_wifi_try_connect(lua_State* L)
{
  uint8_t arg_num = lua_check_top(L, {2, 3, 4});
  if (arg_num == 0)
    return 0; // Не повертаємо нічого бо функція вище, вже запушила помилку

  const char* ssid = luaL_checkstring(L, 1);
  const char* pwd = luaL_checkstring(L, 2);
  int wifi_chan = 1;
  bool autoreconnect = false;

  if (arg_num > 2)
  {
    wifi_chan = luaL_checkinteger(L, 3);

    if (arg_num > 3)
      autoreconnect = lua_toboolean(L, 4);
  }

  lua_pushboolean(L, pixeler::_wifi.tryConnectTo(ssid, pwd, wifi_chan, autoreconnect));

  return 1;
}

int lua_wifi_create_ap(lua_State* L)
{
  uint8_t arg_num = lua_check_top(L, {2, 3, 4, 5});
  if (arg_num == 0)
    return 0; // Не повертаємо нічого бо функція вище, вже запушила помилку

  const char* ssid = luaL_checkstring(L, 1);
  const char* pwd = luaL_checkstring(L, 2);
  int max_connection = 1;
  int wifi_chan = 1;
  bool is_hidden = false;

  if (arg_num > 2)
  {
    max_connection = luaL_checkinteger(L, 3);

    if (arg_num > 3)
    {
      wifi_chan = luaL_checkinteger(L, 4);

      if (arg_num > 4)
        is_hidden = lua_toboolean(L, 5);
    }
  }

  lua_pushboolean(L, pixeler::_wifi.createAP(ssid, pwd, max_connection, wifi_chan, is_hidden));

  return 1;
}

int lua_wifi_set_power(lua_State* L)
{
  int power = luaL_checkinteger(L, 1);
  pixeler::_wifi.setPower(static_cast<pixeler::WiFiManager::WiFiPowerLevel>(power));
  return 0;
}

int lua_wifi_is_connected(lua_State* L)
{
  lua_pushboolean(L, pixeler::_wifi.isConnected());
  return 1;
}

int lua_wifi_is_ap_enabled(lua_State* L)
{
  lua_pushboolean(L, pixeler::_wifi.isApEnabled());
  return 1;
}

int lua_wifi_get_ssid(lua_State* L)
{
  lua_pushstring(L, pixeler::_wifi.getSSID().c_str());
  return 1;
}

int lua_wifi_disconnect(lua_State* L)
{
  pixeler::_wifi.disconnect();
  return 0;
}

int lua_wifi_is_enabled(lua_State* L)
{
  lua_pushboolean(L, pixeler::_wifi.isEnabled());
  return 1;
}

int lua_wifi_enable(lua_State* L)
{
  lua_pushboolean(L, pixeler::_wifi.enable());
  return 1;
}

int lua_wifi_disable(lua_State* L)
{
  pixeler::_wifi.disable();
  return 0;
}

int lua_wifi_get_ip(lua_State* L)
{
  lua_pushstring(L, pixeler::_wifi.getIP().c_str());
  return 1;
}

int lua_wifi_is_busy(lua_State* L)
{
  lua_pushboolean(L, pixeler::_wifi.isBusy());
  return 1;
}

const struct luaL_Reg LIB_WIFI[] = {
    {"tryConnectTo", lua_wifi_try_connect},
    {"createAP", lua_wifi_create_ap},
    {"setPower", lua_wifi_set_power},
    {"isConnected", lua_wifi_is_connected},
    {"isApEnabled", lua_wifi_is_ap_enabled},
    {"getSSID", lua_wifi_get_ssid},
    {"disconnect", lua_wifi_disconnect},
    {"isEnabled", lua_wifi_is_enabled},
    {"enable", lua_wifi_enable},
    {"disable", lua_wifi_disable},
    {"getIP", lua_wifi_get_ip},
    {"isBusy", lua_wifi_is_busy},
    {nullptr, nullptr},
};

int lua_open_wifi(lua_State* L)
{
  luaL_newlib(L, LIB_WIFI);
  return 1;
}