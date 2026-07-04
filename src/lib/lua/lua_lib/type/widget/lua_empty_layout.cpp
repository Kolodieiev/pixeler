#pragma GCC optimize("O3")
#include "lua_empty_layout.h"

#include "./lua_iwidget_cont.h"
#include "lib/lua/res/lua_strs.h"
#include "widget/layout/EmptyLayout.h"

using namespace pixeler;

int lua_el_new(lua_State* L)
{
  uint16_t id = luaL_checkinteger(L, 2);
  EmptyLayout** widet = static_cast<EmptyLayout**>(lua_newuserdata(L, sizeof(EmptyLayout*)));
  *widet = new EmptyLayout(id);
  luaL_getmetatable(L, STR_TYPE_NAME_EMPTY_LAYOUT);
  lua_setmetatable(L, -2);
  return 1;
}

int lua_el_clone(lua_State* L)
{
  EmptyLayout* el = *static_cast<EmptyLayout**>(luaL_checkudata(L, 1, STR_TYPE_NAME_EMPTY_LAYOUT));
  uint16_t id = luaL_checkinteger(L, 2);
  EmptyLayout* clone = el->clone(id);

  EmptyLayout** el_clone = static_cast<EmptyLayout**>(lua_newuserdata(L, sizeof(EmptyLayout*)));
  *el_clone = clone;

  luaL_getmetatable(L, STR_TYPE_NAME_EMPTY_LAYOUT);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_el_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_TYPE_NAME_EMPTY_LAYOUT);
  return 0;
}

const struct luaL_Reg TYPE_METH_EMPTY_LAYOUT[] = {
    {STR_LUA_WIDGET_CLONE, lua_el_clone},
    {STR_LUA_UNLOAD, lua_el_unload},
    {nullptr, nullptr},
};

void lua_init_empty_layout(lua_State* L)
{
  luaL_newmetatable(L, STR_TYPE_NAME_EMPTY_LAYOUT);
  lua_newtable(L);
  luaL_setfuncs(L, TYPE_METH_EMPTY_LAYOUT, 0);
  luaL_getmetatable(L, STR_TYPE_NAME_IWIDGET_CONT);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, STR_LUA_INDEX);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushcfunction(L, lua_el_new);
  lua_setfield(L, -2, STR_LUA_NEW);
  lua_setglobal(L, STR_TYPE_NAME_EMPTY_LAYOUT);
}
