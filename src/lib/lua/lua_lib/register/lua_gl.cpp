#pragma GCC optimize("O3")
#include "lua_gl.h"

#include <stdint.h>

#include <cstring>

#include "lib/lua/lua_lib/helper/lua_helper.h"
#include "driver/graphics/DisplayWrapper.h"

using namespace pixeler;

const char STR_LIB_NAME_GL[] = "gl";

int lua_gl_width(lua_State* L)
{
#ifdef GRAPHICS_ENABLED
  lua_pushinteger(L, UI_WIDTH);
#else
  lua_pushinteger(L, 0);
#endif  // #ifdef GRAPHICS_ENABLED
  return 1;
}

int lua_gl_height(lua_State* L)
{
#ifdef GRAPHICS_ENABLED
  lua_pushinteger(L, UI_HEIGHT);
#else
  lua_pushinteger(L, 0);
#endif  // #ifdef GRAPHICS_ENABLED
  return 1;
}

int lua_gl_draw_rect(lua_State* L)
{
  if (lua_check_top(L, {5}) == 0)
    return 0;

  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  int c = luaL_checkinteger(L, 5);

  _display.drawRect(x, y, w, h, c);
  return 0;
}

int lua_gl_fill_rect(lua_State* L)
{
  if (lua_check_top(L, {5}) == 0)
    return 0;

  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  int c = luaL_checkinteger(L, 5);

  _display.fillRect(x, y, w, h, c);
  return 0;
}

int lua_gl_draw_r_rect(lua_State* L)
{
  if (lua_check_top(L, {6}) == 0)
    return 0;

  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  int r = luaL_checkinteger(L, 5);
  int c = luaL_checkinteger(L, 6);

  _display.drawRoundRect(x, y, w, h, r, c);
  return 1;
}

int lua_gl_fill_r_rect(lua_State* L)
{
  if (lua_check_top(L, {6}) == 0)
    return 0;

  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  int r = luaL_checkinteger(L, 5);
  int c = luaL_checkinteger(L, 6);

  _display.fillRoundRect(x, y, w, h, r, c);
  return 1;
}

int lua_gl_set_font(lua_State* L)
{
  const char* font_name = luaL_checkstring(L, 1);
  const uint8_t* font = fontNameToFont(font_name);
  _display.setFont(font);
  return 0;
}

int lua_gl_set_text_size(lua_State* L)
{
  int txt_size = luaL_checkinteger(L, 1);
  _display.setTextSize(txt_size);
  return 0;
}

int lua_gl_set_text_color(lua_State* L)
{
  int color = luaL_checkinteger(L, 1);
  _display.setTextColor(color);
  return 0;
}

int lua_gl_print(lua_State* L)
{
  if (lua_check_top(L, {3}) == 0)
    return 0;

  const char* str = luaL_checkstring(L, 1);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);

  _display.setCursor(x, y);
  _display.print(str);
  return 0;
}

//----------------------------------------------------------------------------------------------------

const struct luaL_Reg LIB_GL[] = {
    {"width", lua_gl_width},
    {"height", lua_gl_height},
    {"drawRect", lua_gl_draw_rect},
    {"fillRect", lua_gl_fill_rect},
    {"drawRoundRect", lua_gl_draw_r_rect},
    {"fillRoundRect", lua_gl_fill_r_rect},
    {"setFont", lua_gl_set_font},
    {"setTextSize", lua_gl_set_text_size},
    {"setTextColor", lua_gl_set_text_color},
    {"print", lua_gl_print},
    {nullptr, nullptr},
};

int lua_register_gl(lua_State* L)
{
  luaL_newlib(L, LIB_GL);
  lua_setglobal(L, STR_LIB_NAME_GL);
  return 0;
}