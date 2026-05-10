#pragma GCC optimize("O3")
#include "lua_uart.h"

#include <HardwareSerial.h>

#include "../helper/lua_helper.h"

HardwareSerial& get_serial_by_num(uint32_t serial_num)
{
  if (serial_num > SOC_UART_NUM)
  {
    log_e("На чіпі доступно лише %u UART. Запитується %u-й", SOC_UART_NUM, serial_num);
    return Serial0;
  }

  if (serial_num == 0)
    return Serial0;
#if SOC_UART_NUM > 1
  else if (serial_num == 1)
    return Serial1;
#endif
#if SOC_UART_NUM > 2
  else
    return Serial2;
#endif
}

int lua_uart_begin(lua_State* L)
{
  uint8_t arg_num = lua_check_top(L, {2, 4, 5});
  if (arg_num == 0)
    return 0;

  int rx = -1;
  int tx = -1;
  bool invert = false;

  if (arg_num > 2)
  {
    rx = luaL_checkinteger(L, 3);
    tx = luaL_checkinteger(L, 4);

    if (arg_num == 5)
      invert = lua_toboolean(L, 5);
  }

  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  int baud = luaL_checkinteger(L, 2);
  serial.begin(baud, SERIAL_8N1, rx, tx, invert);

  return 0;
}

int lua_uart_end(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  serial.end();
  return 0;
}

int lua_uart_update_baud_rate(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  int baud = luaL_checkinteger(L, 2);
  serial.updateBaudRate(baud);
  return 0;
}

int lua_uart_available(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  lua_pushinteger(L, serial.available());
  return 1;
}

int lua_uart_peek(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  int peek = serial.peek();

  if (peek == -1)
  {
    lua_pushnil(L);
  }
  else
  {
    char c = static_cast<char>(peek);
    lua_pushlstring(L, &c, 1);
  }

  return 1;
}

int lua_uart_read(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  int read_ch = serial.read();

  if (read_ch == -1)
  {
    lua_pushnil(L);
  }
  else
  {
    char c = static_cast<char>(read_ch);
    lua_pushlstring(L, &c, 1);
  }

  return 1;
}

int lua_uart_print(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  const char* str = luaL_checkstring(L, 2);
  size_t printed = serial.print(str);
  lua_pushinteger(L, printed);
  return 1;
}

int lua_uart_println(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  const char* str = luaL_checkstring(L, 2);
  size_t printed = serial.println(str);
  lua_pushinteger(L, printed);
  return 1;
}

int lua_uart_flush(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  serial.flush();
  return 0;
}

int lua_uart_set_rx_buf_size(lua_State* L)
{
  HardwareSerial& serial = get_serial_by_num(luaL_checkinteger(L, 1));
  int buff_size = luaL_checkinteger(L, 2);
  size_t new_size = serial.setRxBufferSize(buff_size);
  lua_pushinteger(L, new_size);
  return 1;
}

const struct luaL_Reg LIB_UART[] = {
    {"begin", lua_uart_begin},
    {"end", lua_uart_end},
    {"updateBaudRate", lua_uart_update_baud_rate},
    {"available", lua_uart_available},
    {"peek", lua_uart_peek},
    {"read", lua_uart_read},
    {"print", lua_uart_print},
    {"println", lua_uart_println},
    {"flush", lua_uart_flush},
    {"setRxBuffSize", lua_uart_set_rx_buf_size},
    {nullptr, nullptr},
};

int lua_open_uart(lua_State* L)
{
  luaL_newlib(L, LIB_UART);
  return 1;
}