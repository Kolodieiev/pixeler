#pragma GCC optimize("O3")
#include "lua_label.h"

#include "./lua_iwidget.h"
#include "lua_image.h"
#include "lib/lua/lua_lib/helper/lua_helper.h"
#include "lib/lua/res/lua_strs.h"
#include "widget/image/Image.h"
#include "widget/text/Label.h"

using namespace pixeler;

int lua_label_new(lua_State* L)
{
  uint16_t id = luaL_checkinteger(L, 2);
  Label** widet = static_cast<Label**>(lua_newuserdata(L, sizeof(Label*)));
  *widet = new Label(id);
  luaL_getmetatable(L, STR_TYPE_NAME_LABEL);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_label_clone(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t id = luaL_checkinteger(L, 2);
  Label* clone = label->clone(id);

  Label** label_clone = static_cast<Label**>(lua_newuserdata(L, sizeof(Label*)));
  *label_clone = clone;

  luaL_getmetatable(L, STR_TYPE_NAME_LABEL);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_label_init_width_to_fit(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t add_val = luaL_checknumber(L, 2);
  label->initWidthToFit(add_val);
  return 0;
}

int lua_label_update_width_to_fit(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t add_val = luaL_checknumber(L, 2);
  label->updateWidthToFit(add_val);
  return 0;
}

int lua_label_set_text(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  const char* text = luaL_checkstring(L, 2);
  label->setText(text);
  return 0;
}

int lua_label_get_text(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  lua_pushstring(L, label->getText().c_str());
  return 1;
}

int lua_label_set_text_size(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint8_t size = luaL_checkinteger(L, 2);
  label->setTextSize(size);
  return 0;
}

int lua_label_set_text_color(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t color = luaL_checkinteger(L, 2);
  label->setTextColor(color);
  return 0;
}

int lua_label_set_font(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  const char* font_name = luaL_checkstring(L, 2);
  label->setFont(fontNameToFont(font_name));
  return 0;
}

int lua_label_set_gravity(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t raw_value = luaL_checkinteger(L, 2);
  if (raw_value > IWidget::GRAVITY_BOTTOM)
    return luaL_error(L, "Invalid gravity value: %d", raw_value);

  IWidget::Gravity gravity = static_cast<IWidget::Gravity>(raw_value);
  label->setGravity(gravity);
  return 0;
}

int lua_label_set_align(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint16_t raw_value = luaL_checkinteger(L, 2);
  if (raw_value > IWidget::ALIGN_END)
    return luaL_error(L, "Invalid alignment value: %d", raw_value);

  IWidget::Alignment align = static_cast<IWidget::Alignment>(raw_value);
  label->setAlign(align);
  return 0;
}

int lua_label_set_h_padding(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  uint8_t padding = luaL_checkinteger(L, 2);
  label->setHPadding(padding);
  return 0;
}

int lua_label_get_text_len(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  lua_pushinteger(L, label->getLen());
  return 1;
}

int lua_label_set_autoscroll(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  bool state = lua_toboolean(L, 2);
  label->setAutoscroll(state);
  return 0;
}

int lua_label_set_autoscroll_in_focus(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  bool state = lua_toboolean(L, 2);
  label->setAutoscrollInFocus(state);
  return 0;
}

int lua_label_set_back_img(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  Image** image = static_cast<Image**>(luaL_checkudata(L, 2, STR_TYPE_NAME_IMAGE));
  label->setBackImg(*image);

  return 0;
}

int lua_label_set_multiline(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  bool state = lua_toboolean(L, 2);
  label->setMultiline(state);
  return 0;
}

int lua_label_set_autoscroll_delay(lua_State* L)
{
  Label* label = *static_cast<Label**>(luaL_checkudata(L, 1, STR_TYPE_NAME_LABEL));
  int delay = luaL_checkinteger(L, 2);
  label->setAutoscrollDelay(delay);
  return 0;
}

int lua_label_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_TYPE_NAME_LABEL);
  return 0;
}

const struct luaL_Reg TYPE_METH_LABEL[] = {
    {"initWidthToFit", lua_label_init_width_to_fit},
    {"updateWidthToFit", lua_label_update_width_to_fit},
    {"setText", lua_label_set_text},
    {"getText", lua_label_get_text},
    {"setTextSize", lua_label_set_text_size},
    {"setTextColor", lua_label_set_text_color},
    {"setFont", lua_label_set_font},
    {"setGravity", lua_label_set_gravity},
    {"setAlign", lua_label_set_align},
    {"setHPadding", lua_label_set_h_padding},
    {"getLen", lua_label_get_text_len},
    {"setAutoscroll", lua_label_set_autoscroll},
    {"setAutoscrollInFocus", lua_label_set_autoscroll_in_focus},
    {"setAutoscrollDelay", lua_label_set_autoscroll_delay},
    {"setBackImg", lua_label_set_back_img},
    {"setMultiline", lua_label_set_multiline},
    {STR_LUA_WIDGET_CLONE, lua_label_clone},
    {STR_LUA_UNLOAD, lua_label_unload},
    {nullptr, nullptr},
};

void lua_init_label(lua_State* L)
{
  luaL_newmetatable(L, STR_TYPE_NAME_LABEL);
  lua_newtable(L);
  luaL_setfuncs(L, TYPE_METH_LABEL, 0);
  luaL_getmetatable(L, STR_TYPE_NAME_IWIDGET);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, STR_LUA_INDEX);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushcfunction(L, lua_label_new);
  lua_setfield(L, -2, STR_LUA_NEW);
  lua_setglobal(L, STR_TYPE_NAME_LABEL);
}
