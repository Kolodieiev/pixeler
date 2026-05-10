#pragma GCC optimize("O3")
#include "lua_sd.h"

#include <stdint.h>

#include "manager/FileManager.h"
#include "lib/lua/res/lua_strs.h"

const char STR_FILE_TYPE[] = "FILE*";
//
const char STR_MEM_ALLOC_ERR[] = "Помилка створення буфера";
const char STR_FILE_READ_ERR[] = "Помилка читання файла";
//

int lua_sd_file_exist(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, pixeler::_fs.fileExist(path, true));
  return 1;
}

int lua_sd_dir_exist(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, pixeler::_fs.dirExist(path, true));
  return 1;
}

int lua_sd_open_file(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  const char* mode = luaL_checkstring(L, 2);

  FILE* fp = pixeler::_fs.openFile(path, mode);

  if (!fp)
  {
    lua_pushnil(L);
  }
  else
  {
    FILE** f = static_cast<FILE**>(lua_newuserdata(L, sizeof(FILE*)));
    *f = fp;
    luaL_getmetatable(L, STR_FILE_TYPE);
    lua_setmetatable(L, -2);
  }

  return 1;
}

int lua_sd_close_file(lua_State* L)
{
  FILE** f = static_cast<FILE**>(luaL_checkudata(L, 1, STR_FILE_TYPE));
  pixeler::_fs.closeFile(*f);
  return 0;
}

int lua_sd_file_size(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  lua_pushinteger(L, pixeler::_fs.getFileSize(path));
  return 1;
}

int lua_sd_available(lua_State* L)
{
  FILE** f = static_cast<FILE**>(luaL_checkudata(L, 1, STR_FILE_TYPE));
  int size = luaL_checkinteger(L, 2);

  lua_pushinteger(L, pixeler::_fs.available(*f, size));
  return 1;
}

int lua_sd_read_file(lua_State* L)
{
  bool has_seek_pos = lua_gettop(L) == 3;

  const char* path = luaL_checkstring(L, 1);
  int len = luaL_checkinteger(L, 2);
  int seek_pos = 0;
  if (has_seek_pos)
    seek_pos = luaL_checkinteger(L, 3);

  void* buffer;
  if (psramInit())
    buffer = ps_malloc(len);
  else
    buffer = malloc(len);

  if (!buffer)
    return luaL_error(L, STR_MEM_ALLOC_ERR);

  size_t bytes_read = pixeler::_fs.readFile(path, buffer, len, seek_pos);

  lua_pushlstring(L, (char*)buffer, bytes_read);
  free(buffer);
  return 1;
}

int lua_sd_read_from_file(lua_State* L)
{
  bool has_seek_pos = lua_gettop(L) == 3;

  FILE** f = static_cast<FILE**>(luaL_checkudata(L, 1, STR_FILE_TYPE));
  int len = luaL_checkinteger(L, 2);
  int seek_pos = 0;
  if (has_seek_pos)
    seek_pos = luaL_checkinteger(L, 3);

  char* buffer;
  if (psramInit())
    buffer = static_cast<char*>(ps_malloc(len));
  else
    buffer = static_cast<char*>(malloc(len));

  if (!buffer)
    return luaL_error(L, STR_MEM_ALLOC_ERR);

  if (!pixeler::_fs.readFromFileExact(*f, buffer, len, seek_pos))
  {
    free(buffer);
    return luaL_error(L, STR_FILE_READ_ERR);
  }

  lua_pushlstring(L, (char*)buffer, len);
  free(buffer);

  return 1;
}

int lua_sd_write_file(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  size_t len;
  const char* buf = luaL_checklstring(L, 2, &len);

  size_t bytes_write = pixeler::_fs.writeFile(path, buf, len);

  lua_pushboolean(L, bytes_write == len);

  return 1;
}

int lua_sd_write_to_file(lua_State* L)
{
  FILE** f = static_cast<FILE**>(luaL_checkudata(L, 1, STR_FILE_TYPE));
  size_t len;
  const char* buf = luaL_checklstring(L, 2, &len);

  size_t bytes_write = pixeler::_fs.writeToFile(*f, buf, len);

  lua_pushboolean(L, bytes_write == len);

  return 1;
}

int lua_sd_copy_file(lua_State* L)
{
  const char* from_path = luaL_checkstring(L, 1);
  const char* to_path = luaL_checkstring(L, 2);

  bool result = false;
  if (pixeler::_fs.startCopyingFile(from_path, to_path))
  {
    while (pixeler::_fs.isWorking())
      delay(2);

    result = pixeler::_fs.lastTaskResult();
  }

  lua_pushboolean(L, result);

  return 1;
}

int lua_sd_remove(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  bool result = false;
  if (pixeler::_fs.startRemoving(path))
  {
    while (pixeler::_fs.isWorking())
      delay(2);

    result = pixeler::_fs.lastTaskResult();
  }

  lua_pushboolean(L, result);

  return 1;
}

int lua_sd_rename(lua_State* L)
{
  const char* old_name = luaL_checkstring(L, 1);
  const char* new_name = luaL_checkstring(L, 2);

  lua_pushboolean(L, pixeler::_fs.rename(old_name, new_name));

  return 1;
}

int lua_sd_create_dir(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);

  lua_pushboolean(L, pixeler::_fs.createDir(path));
  return 1;
}

int lua_sd_unload(lua_State* L)
{
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, STR_FILE_TYPE);
  return 0;
}

const struct luaL_Reg LIB_SD[] = {
    {"fileExist", lua_sd_file_exist},
    {"dirExist", lua_sd_dir_exist},
    {"open", lua_sd_open_file},
    {"close", lua_sd_close_file},
    {"fileSize", lua_sd_file_size},
    {"available", lua_sd_available},
    {"read", lua_sd_read_file},
    {"readFrom", lua_sd_read_from_file},
    {"write", lua_sd_write_file},
    {"writeTo", lua_sd_write_to_file},
    {"copyFile", lua_sd_copy_file},
    {"remove", lua_sd_remove},
    {"rename", lua_sd_rename},
    {"mkDir", lua_sd_create_dir},
    {STR_LUA_UNLOAD, lua_sd_unload},
    {nullptr, nullptr},
};

int lua_open_sd(lua_State* L)
{
  luaL_newlib(L, LIB_SD);
  luaL_newmetatable(L, STR_FILE_TYPE);
  lua_pop(L, 1);
  return 1;
}