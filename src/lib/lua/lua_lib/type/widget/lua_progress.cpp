#pragma GCC optimize("O3")
#include "lua_progress.h"

#include "./lua_iwidget.h"
#include "lib/lua/res/lua_strs.h"
#include "widget/progress/ProgressBar.h"

using namespace pixeler;

int lua_progress_new(lua_State* L)
{
  uint16_t id = luaL_checkinteger(L, 2);
  ProgressBar** widet = static_cast<ProgressBar**>(lua_newuserdata(L, sizeof(ProgressBar*)));
  *widet = new ProgressBar(id);
  luaL_getmetatable(L, STR_TYPE_NAME_PROGRESS);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_progress_clone(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  uint16_t id = luaL_checkinteger(L, 2);
  ProgressBar* clone = progress->clone(id);

  ProgressBar** progress_clone = static_cast<ProgressBar**>(lua_newuserdata(L, sizeof(ProgressBar*)));
  *progress_clone = clone;

  luaL_getmetatable(L, STR_TYPE_NAME_PROGRESS);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_progress_set_max(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  uint16_t max = luaL_checkinteger(L, 2);
  progress->setMax(max);
  return 0;
}

int lua_progress_get_max(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  lua_pushinteger(L, progress->getMax());
  return 1;
}

int lua_progress_set_progress(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  uint16_t pr_val = luaL_checkinteger(L, 2);
  progress->setProgress(pr_val);
  return 0;
}

int lua_progress_get_progress(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  lua_pushinteger(L, progress->getProgress());
  return 1;
}

int lua_progress_set_progress_color(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  uint16_t color = luaL_checkinteger(L, 2);
  progress->setProgressColor(color);
  return 0;
}

int lua_progress_set_orientation(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  uint16_t raw_value = luaL_checkinteger(L, 2);
  if (raw_value > IWidget::VERTICAL)
    return luaL_error(L, "Invalid orientation value: %d", raw_value);

  IWidget::Orientation orient = static_cast<IWidget::Orientation>(raw_value);
  progress->setOrientation(orient);
  return 0;
}

int lua_progress_reset(lua_State* L)
{
  ProgressBar* progress = *static_cast<ProgressBar**>(luaL_checkudata(L, 1, STR_TYPE_NAME_PROGRESS));
  progress->reset();
  return 0;
}

int lua_progress_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_TYPE_NAME_PROGRESS);
  return 0;
}

const struct luaL_Reg TYPE_METH_PROGRESS[] = {
    {"setMax", lua_progress_set_max},
    {"getMax", lua_progress_get_max},
    {"setProgress", lua_progress_set_progress},
    {"getProgress", lua_progress_get_progress},
    {"setProgressColor", lua_progress_set_progress_color},
    {"setOrientation", lua_progress_set_orientation},
    {"reset", lua_progress_reset},
    {STR_LUA_WIDGET_CLONE, lua_progress_clone},
    {STR_LUA_UNLOAD, lua_progress_unload},
    {nullptr, nullptr},
};

void lua_init_progress(lua_State* L)
{
  luaL_newmetatable(L, STR_TYPE_NAME_PROGRESS);
  lua_newtable(L);
  luaL_setfuncs(L, TYPE_METH_PROGRESS, 0);
  luaL_getmetatable(L, STR_TYPE_NAME_IWIDGET);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, STR_LUA_INDEX);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushcfunction(L, lua_progress_new);
  lua_setfield(L, -2, STR_LUA_NEW);
  lua_setglobal(L, STR_TYPE_NAME_PROGRESS);
}
