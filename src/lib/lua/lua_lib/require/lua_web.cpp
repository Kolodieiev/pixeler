#pragma GCC optimize("O3")
#include "lua_web.h"

#include <HTTPClient.h>

#include "manager/WiFiManager.h"

const char STR_LUA_WEB_NOT_CONN_ERR[] = "Не приєднано до мережі";
const char STR_LUA_WEB_INCORRECT_URL[] = "Некоректна URL";

void push_get_resp_to_lua(const String& url, HTTPClient& http, int http_code, lua_State* L)
{
  if (http_code != HTTP_CODE_OK)
  {
    log_e("Отримали помилку за запитом: %s", url.c_str());
    lua_pushstring(L, emptyString.c_str());
  }
  else
  {
    String payload = http.getString();
    lua_pushstring(L, payload.c_str());
  }
}

void push_post_resp_to_lua(const String& url, const String& req_data, HTTPClient& http, int http_code, lua_State* L)
{
  if (http_code != HTTP_CODE_OK)
  {
    log_e("Отримали помилку за запитом: %s\n З даними: ", url.c_str(), req_data.c_str());
    lua_pushstring(L, emptyString.c_str());
  }
  else
  {
    String payload = http.getString();
    lua_pushstring(L, payload.c_str());
  }
}

int lua_web_get(lua_State* L)
{
  if (!pixeler::_wifi.isConnected())
    return luaL_error(L, STR_LUA_WEB_NOT_CONN_ERR);

  String url{luaL_checkstring(L, 1)};

  HTTPClient http;

  if (!http.begin(url))
    return luaL_error(L, "%s: %s", STR_LUA_WEB_INCORRECT_URL, url.c_str());

  int http_code = http.GET();

  push_get_resp_to_lua(url, http, http_code, L);

  http.end();
  return 1;
}

int lua_web_post(lua_State* L)
{
  if (!pixeler::_wifi.isConnected())
    return luaL_error(L, STR_LUA_WEB_NOT_CONN_ERR);

  String url{luaL_checkstring(L, 1)};
  String req_data{luaL_checkstring(L, 2)};

  HTTPClient http;
  if (!http.begin(url))
    return luaL_error(L, "%s: %s", STR_LUA_WEB_INCORRECT_URL, url.c_str());

  http.addHeader("Content-Type", "text/plain");

  int http_code = http.POST(req_data);

  push_post_resp_to_lua(url, req_data, http, http_code, L);

  http.end();
  return 1;
}

int lua_web_json_post(lua_State* L)
{
  if (!pixeler::_wifi.isConnected())
    return luaL_error(L, STR_LUA_WEB_NOT_CONN_ERR);

  String url{luaL_checkstring(L, 1)};
  String req_data{luaL_checkstring(L, 2)};
  // "{\"device\":\"ESP32\",\"temperature\":25.5,\"humidity\":60,\"status\":\"active\"}"

  HTTPClient http;
  if (!http.begin(url))
    return luaL_error(L, "%s: %s", STR_LUA_WEB_INCORRECT_URL, url.c_str());

  http.addHeader("Content-Type", "application/json");

  int http_code = http.POST(req_data);

  push_post_resp_to_lua(url, req_data, http, http_code, L);

  http.end();
  return 1;
}

int lua_web_form_post(lua_State* L)
{
  if (!pixeler::_wifi.isConnected())
    return luaL_error(L, STR_LUA_WEB_NOT_CONN_ERR);

  String url{luaL_checkstring(L, 1)};
  String req_data{luaL_checkstring(L, 2)};
  // "username=esp32user&password=secret123&temperature=25.5"

  HTTPClient http;
  if (!http.begin(url))
    return luaL_error(L, "%s: %s", STR_LUA_WEB_INCORRECT_URL, url.c_str());

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int http_code = http.POST(req_data);

  push_post_resp_to_lua(url, req_data, http, http_code, L);

  http.end();
  return 1;
}

const struct luaL_Reg LIB_NET[] = {
    {"get", lua_web_get},
    {"post", lua_web_post},
    {"jsonPost", lua_web_json_post},
    {"formPost", lua_web_form_post},
    {nullptr, nullptr},
};

int lua_open_web(lua_State* L)
{
  luaL_newlib(L, LIB_NET);
  return 1;
}