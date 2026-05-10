#pragma GCC optimize("O3")
#include "lua_helper.h"

#include "driver/graphics/DisplayWrapper.h"

const char STR_INCORRECT_ARGS_NUMBER_BTW[] = "Некоректна кількість аргументів. Очікується [%s], отримано %d";

uint8_t lua_check_top(lua_State* L, std::initializer_list<uint8_t> args_assert)
{
  uint8_t args_num = lua_gettop(L);
  bool result = false;

  for (uint8_t a : args_assert)
  {
    if (args_num == a)
    {
      result = true;
      break;
    }
  }

  if (!result)
  {
    char expected_buf[64] = {0};
    char* ptr = expected_buf;
    for (uint8_t a : args_assert)
      ptr += snprintf(ptr, expected_buf + sizeof(expected_buf) - ptr, "%u,", a);
    if (ptr != expected_buf && ptr[-1] == ',')
      ptr[-1] = '\0';

    return luaL_error(L, STR_INCORRECT_ARGS_NUMBER_BTW, expected_buf, args_num);
  }

  return args_num;
}

const uint8_t* fontNameToFont(const char* font_name)
{
  if (!font_name)
    return font_unifont;

  if (strcmp(font_name, "font_4x6") == 0)
    return font_4x6;
  else if (strcmp(font_name, "font_5x7") == 0)
    return font_5x7;
  else if (strcmp(font_name, "font_5x8") == 0)
    return font_5x8;
  else if (strcmp(font_name, "font_6x12") == 0)
    return font_6x12;
  else if (strcmp(font_name, "font_6x13") == 0)
    return font_6x13;
  else if (strcmp(font_name, "font_6x13B") == 0)
    return font_6x13B;
  else if (strcmp(font_name, "font_7x13") == 0)
    return font_7x13;
  else if (strcmp(font_name, "font_8x13") == 0)
    return font_8x13;
  else if (strcmp(font_name, "font_9x15") == 0)
    return font_9x15;
  else if (strcmp(font_name, "font_10x20") == 0)
    return font_10x20;
  else if (strcmp(font_name, "font_cu12") == 0)
    return font_cu12;
  else if (strcmp(font_name, "font_inr24") == 0)
    return font_inr24;
  else if (strcmp(font_name, "font_inr27") == 0)
    return font_inr27;
  else if (strcmp(font_name, "font_inr30") == 0)
    return font_inr30;
  else if (strcmp(font_name, "font_inr33") == 0)
    return font_inr33;
  else if (strcmp(font_name, "font_inr38") == 0)
    return font_inr38;
  else if (strcmp(font_name, "font_inr42") == 0)
    return font_inr42;
  else if (strcmp(font_name, "font_inr46") == 0)
    return font_inr46;
  else if (strcmp(font_name, "font_inr49") == 0)
    return font_inr49;
  else if (strcmp(font_name, "font_inr53") == 0)
    return font_inr53;

  return font_unifont;
}