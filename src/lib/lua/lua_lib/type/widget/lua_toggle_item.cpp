#pragma GCC optimize("O3")
#include "lua_toggle_item.h"

#include "./lua_menu_item.h"
#include "./lua_toggle_switch.h"
#include "lib/lua/res/lua_strs.h"
#include "widget/menu/item/ToggleItem.h"
#include "widget/toggle/ToggleSwitch.h"

using namespace pixeler;

int lua_toggle_item_new(lua_State* L)
{
  uint16_t id = luaL_checkinteger(L, 2);
  ToggleItem** widet = static_cast<ToggleItem**>(lua_newuserdata(L, sizeof(ToggleItem*)));
  *widet = new ToggleItem(id);
  luaL_getmetatable(L, STR_TYPE_NAME_TOGGLE_ITEM);
  lua_setmetatable(L, -2);
  return 1;
}

int lua_toggle_item_clone(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  uint16_t id = luaL_checkinteger(L, 2);
  ToggleItem* clone = toggle_item->clone(id);

  ToggleItem** toggle_item_clone = static_cast<ToggleItem**>(lua_newuserdata(L, sizeof(ToggleItem*)));
  *toggle_item_clone = clone;

  luaL_getmetatable(L, STR_TYPE_NAME_TOGGLE_ITEM);
  lua_setmetatable(L, -2);
  return 1;
}

int lua_toggle_item_set_toggle(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  ToggleSwitch* toggle_switch = *static_cast<ToggleSwitch**>(luaL_checkudata(L, 2, STR_TYPE_NAME_TOGGLE_SWITCH));
  toggle_item->setToggle(toggle_switch);
  return 0;
}

int lua_toggle_item_get_toggle(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  ToggleSwitch** lua_toggle_switch = static_cast<ToggleSwitch**>(lua_newuserdata(L, sizeof(ToggleSwitch*)));
  *lua_toggle_switch = toggle_item->getToggle();

  luaL_getmetatable(L, STR_TYPE_NAME_TOGGLE_SWITCH);
  lua_setmetatable(L, -2);
  return 1;
}

int lua_toggle_item_set_on(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  bool state = lua_toboolean(L, 2);
  toggle_item->setOn(state);
  return 0;
}

int lua_toggle_item_toggle(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  toggle_item->toggle();
  return 0;
}

int lua_toggle_item_is_on(lua_State* L)
{
  ToggleItem* toggle_item = *static_cast<ToggleItem**>(luaL_checkudata(L, 1, STR_TYPE_NAME_TOGGLE_ITEM));
  lua_pushboolean(L, toggle_item->isOn());
  return 1;
}

int lua_toggle_item_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_TYPE_NAME_TOGGLE_ITEM);
  return 0;
}

const struct luaL_Reg TYPE_METH_TOGGLE_ITEM[] = {
    {"setToggle", lua_toggle_item_set_toggle},
    {"getToggle", lua_toggle_item_get_toggle},
    {"setOn", lua_toggle_item_set_on},
    {"toggle", lua_toggle_item_toggle},
    {"isOn", lua_toggle_item_is_on},
    {STR_LUA_WIDGET_CLONE, lua_toggle_item_clone},
    {STR_LUA_UNLOAD, lua_toggle_item_unload},
    {nullptr, nullptr},
};

void lua_init_toggle_item(lua_State* L)
{
  luaL_newmetatable(L, STR_TYPE_NAME_TOGGLE_ITEM);
  lua_newtable(L);
  luaL_setfuncs(L, TYPE_METH_TOGGLE_ITEM, 0);
  luaL_getmetatable(L, STR_TYPE_NAME_MENU_ITEM);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, STR_LUA_INDEX);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushcfunction(L, lua_toggle_item_new);
  lua_setfield(L, -2, STR_LUA_NEW);
  lua_setglobal(L, STR_TYPE_NAME_TOGGLE_ITEM);
}
