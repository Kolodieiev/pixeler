#pragma GCC optimize("O3")
#include "lua_image.h"

#include "./lua_iwidget.h"
#include "lib/lua/res/lua_strs.h"
#include "manager/ResManager.h"
#include "manager/res/ImageResource.h"
#include "widget/image/Image.h"

using namespace pixeler;

int lua_image_new(lua_State* L)
{
  uint16_t id = luaL_checkinteger(L, 2);
  Image** widet = static_cast<Image**>(lua_newuserdata(L, sizeof(Image*)));
  *widet = new Image(id);
  luaL_getmetatable(L, STR_TYPE_NAME_IMAGE);
  lua_setmetatable(L, -2);
  return 1;
}

int lua_image_clone(lua_State* L)
{
  Image* image = *static_cast<Image**>(luaL_checkudata(L, 1, STR_TYPE_NAME_IMAGE));
  uint16_t id = luaL_checkinteger(L, 2);
  Image* clone = image->clone(id);

  Image** image_clone = static_cast<Image**>(lua_newuserdata(L, sizeof(Image*)));
  *image_clone = clone;

  luaL_getmetatable(L, STR_TYPE_NAME_IMAGE);
  lua_setmetatable(L, -2);

  return 1;
}

int lua_image_set_src(lua_State* L)
{
  Image* image = *static_cast<Image**>(luaL_checkudata(L, 1, STR_TYPE_NAME_IMAGE));
  uint32_t res_id = luaL_checkinteger(L, 2);

  if (res_id == 0)
    return 0;

  const IResource* raw_res = _res.getResByID(res_id);

  if (!raw_res)
    return 0;

  const ImageResource* img_res = static_cast<const ImageResource*>(raw_res);

  image->setSrc(static_cast<const uint16_t*>(img_res->getData()));
  image->setWidth(img_res->getWidth());
  image->setHeight(img_res->getHeight());

  return 0;
}

int lua_image_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_TYPE_NAME_IMAGE);
  return 0;
}

const struct luaL_Reg TYPE_METH_IMAGE[] = {
    {"setSrc", lua_image_set_src},
    {STR_LUA_WIDGET_CLONE, lua_image_clone},
    {STR_LUA_UNLOAD, lua_image_unload},
    {nullptr, nullptr},
};

void lua_init_image(lua_State* L)
{
  luaL_newmetatable(L, STR_TYPE_NAME_IMAGE);
  lua_newtable(L);
  luaL_setfuncs(L, TYPE_METH_IMAGE, 0);
  luaL_getmetatable(L, STR_TYPE_NAME_IWIDGET);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, STR_LUA_INDEX);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushcfunction(L, lua_image_new);
  lua_setfield(L, -2, STR_LUA_NEW);
  lua_setglobal(L, STR_TYPE_NAME_IMAGE);
}
