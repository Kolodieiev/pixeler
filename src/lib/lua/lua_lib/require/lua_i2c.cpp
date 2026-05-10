#pragma GCC optimize("O3")
#include "lua_i2c.h"

#include "bus/I2C_Bus.h"

int lua_i2c_begin(lua_State* L)
{
  lua_pushboolean(L, pixeler::_i2c.begin());
  return 1;
}

int lua_i2c_end(lua_State* L)
{
  pixeler::_i2c.end();
  return 0;
}

int lua_i2c_has_connect(lua_State* L)
{
  int addr = luaL_checkinteger(L, 1);
  lua_pushboolean(L, pixeler::_i2c.hasConnect(addr));
  return 1;
}

int lua_i2c_scan_bus(lua_State* L)
{
  pixeler::_i2c.scanBus();
  return 0;
}

int lua_i2c_write(lua_State* L)
{
  int addr = luaL_checkinteger(L, 1);
  size_t len;
  const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 2, &len);

  lua_pushboolean(L, pixeler::_i2c.write(addr, data, len));
  return 1;
}

int lua_i2c_read(lua_State* L)
{
  int addr = luaL_checkinteger(L, 1);
  int len = luaL_checkinteger(L, 2);

  char* out_data = static_cast<char*>(malloc(len));
  if (!out_data)
    return luaL_error(L, "Помилка алокації %i байтів", len);

  if (!pixeler::_i2c.read(addr, out_data, len))
  {
    log_e("Помилка читання з i2c за адресою %i", addr);
    lua_pushlstring(L, "", 0);
  }
  else
  {
    lua_pushlstring(L, out_data, len);
  }

  free(out_data);
  return 1;
}

int lua_i2c_begin_tr(lua_State* L)
{
  int addr = luaL_checkinteger(L, 1);
  pixeler::_i2c.beginTransmission(addr);
  return 0;
}

int lua_i2c_send(lua_State* L)
{
  size_t len;
  const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 1, &len);
  if (len < 1)
    return luaL_error(L, "Не може приймати порожній рядок");
  else
    lua_pushboolean(L, pixeler::_i2c.send(data, len));

  return 1;
}

int lua_i2c_receive(lua_State* L)
{
  int len = luaL_checkinteger(L, 1);
  if (len < 1)
    return luaL_error(L, "Не може приймати порожній рядок");

  char* out_data = static_cast<char*>(malloc(len));
  if (!out_data)
    return luaL_error(L, "Помилка алокації %i байтів", len);

  lua_pushboolean(L, pixeler::_i2c.receive(out_data, len));

  return 1;
}

int lua_i2c_end_tr(lua_State* L)
{
  lua_pushboolean(L, pixeler::_i2c.endTransmission());
  return 1;
}

const struct luaL_Reg LIB_I2C[] = {
    {"begin", lua_i2c_begin},
    {"end", lua_i2c_end},
    {"hasConnect", lua_i2c_has_connect},
    {"scanBus", lua_i2c_scan_bus},
    {"write", lua_i2c_write},
    {"read", lua_i2c_read},
    {"beginTr", lua_i2c_begin_tr},
    {"send", lua_i2c_send},
    {"receive", lua_i2c_receive},
    {"endTr", lua_i2c_end_tr},
    {nullptr, nullptr},
};

int lua_open_i2c(lua_State* L)
{
  luaL_newlib(L, LIB_I2C);
  return 1;
}