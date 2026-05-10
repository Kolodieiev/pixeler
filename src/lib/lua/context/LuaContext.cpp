#pragma GCC optimize("O3")
#include "LuaContext.h"

#include <esp_heap_caps.h>
//
#include "../lua_lib/helper/lua_helper.h"
#include "../lua_lib/type/widget/lua_empty_layout.h"
#include "../lua_lib/type/widget/lua_image.h"
#include "../lua_lib/type/widget/lua_iwidget.h"
#include "../lua_lib/type/widget/lua_iwidget_cont.h"
#include "../lua_lib/type/widget/lua_label.h"
#include "../lua_lib/type/widget/lua_menu.h"
#include "../lua_lib/type/widget/lua_menu_item.h"
#include "../lua_lib/type/widget/lua_progress.h"
#include "../lua_lib/type/widget/lua_toggle_item.h"
#include "../lua_lib/type/widget/lua_toggle_switch.h"

//---------------------------------------------------------------------------------- Бібліотеки, що будуть доступні в скриптах завжди
#include "../lua_lib/register/lua_gl.h"
#include "../lua_lib/register/lua_mcu.h"
#include "../lua_lib/register/lua_pin.h"
//---------------------------------------------------------------------------------- Бібліотеки, які можна підключати та відключати
#include "../lua_lib/require/lua_i2c.h"
#include "../lua_lib/require/lua_pwm.h"
#include "../lua_lib/require/lua_sd.h"
#include "../lua_lib/require/lua_uart.h"
#include "../lua_lib/require/lua_web.h"
#include "../lua_lib/require/lua_wifi.h"
//---------------------------------------------------------------------------------- Реєстрація користувацьких типів
#include "../lua_lib/type/widget/lua_empty_layout.h"
#include "../lua_lib/type/widget/lua_image.h"
#include "../lua_lib/type/widget/lua_iwidget.h"
#include "../lua_lib/type/widget/lua_iwidget_cont.h"
#include "../lua_lib/type/widget/lua_label.h"
#include "../lua_lib/type/widget/lua_menu.h"
#include "../lua_lib/type/widget/lua_menu_item.h"
#include "../lua_lib/type/widget/lua_progress.h"
#include "../lua_lib/type/widget/lua_toggle_item.h"
#include "../lua_lib/type/widget/lua_toggle_switch.h"
//
#include "manager/ResManager.h"
#include "widget/layout/EmptyLayout.h"

static const char STR_NOTIFICATION[] = "Повідомлення";
static const char STR_OK[] = "OK";
static const char STR_NOT_ENOUGH_RAM[] = "Недостатньо RAM для роботи LuaVM";
static const char STR_LUA_START[] = "LuaVM стартувала";
static const char STR_LUA_STOP[] = "LuaVM зупинено";

static const char STR_UPDATE_NAME[] = "update";
//
namespace pixeler
{

  //---------------------------------------------------------------------------------- Бібліотеки, що будуть доступні в скриптах завжди

  // Додай сюди функції для глобальної реєстрації бібліотек
  const LuaRegisterFunc LuaContext::LIB_REGULAR_MODULE[] = {
      LuaContext::lua_register_context,
      LuaContext::lua_register_input,
      lua_register_mcu,
      lua_register_pin,
      lua_register_gl,
  };

  //---------------------------------------------------------------------------------- Бібліотеки, які можна підключати та відключати

  // Додай сюди функції для підключення бібліотек з допомогою require
  int LuaContext::registerRequire(lua_State* L)
  {
    const char* lib_name = luaL_checkstring(L, 1);
    if (strcmp(lib_name, STR_LIB_NAME_SD) == 0)
      lua_pushcfunction(L, lua_open_sd);
    else if (strcmp(lib_name, STR_LIB_NAME_WIFI) == 0)
      lua_pushcfunction(L, lua_open_wifi);
    else if (strcmp(lib_name, STR_LIB_NAME_NET) == 0)
      lua_pushcfunction(L, lua_open_web);
    else if (strcmp(lib_name, STR_LIB_NAME_UART) == 0)
      lua_pushcfunction(L, lua_open_uart);
    else if (strcmp(lib_name, STR_LIB_NAME_I2C) == 0)
      lua_pushcfunction(L, lua_open_i2c);
    else if (strcmp(lib_name, STR_LIB_NAME_PWM) == 0)
      lua_pushcfunction(L, lua_open_pwm);
    else
      luaL_error(L, "Відсутній завантажувач для модуля: %s", lib_name);

    return 1;
  }

  //---------------------------------------------------------------------------------- Реєстрація користувацьких типів
  /* Назва типу в луа, {список імен типів-залежностей}, функція-ініціалізатор типу */
  const LuaContext::LuaCustomType LuaContext::LIB_CUSTOM_TYPE[] = {
      {STR_TYPE_NAME_IWIDGET, {}, lua_init_iwidget},
      {STR_TYPE_NAME_IWIDGET_CONT, {STR_TYPE_NAME_IWIDGET}, lua_init_iwidget_cont},
      {STR_TYPE_NAME_LABEL, {STR_TYPE_NAME_IWIDGET}, lua_init_label},
      {STR_TYPE_NAME_IMAGE, {STR_TYPE_NAME_IWIDGET}, lua_init_image},
      {STR_TYPE_NAME_EMPTY_LAYOUT, {STR_TYPE_NAME_IWIDGET, STR_TYPE_NAME_IWIDGET_CONT}, lua_init_empty_layout},
      {STR_TYPE_NAME_MENU, {STR_TYPE_NAME_IWIDGET_CONT}, lua_init_menu},
      {STR_TYPE_NAME_MENU_ITEM, {STR_TYPE_NAME_LABEL, STR_TYPE_NAME_IMAGE}, lua_init_menu_item},
      {STR_TYPE_NAME_TOGGLE_ITEM, {STR_TYPE_NAME_MENU_ITEM, STR_TYPE_NAME_TOGGLE_SWITCH}, lua_init_toggle_item},
      {STR_TYPE_NAME_TOGGLE_SWITCH, {STR_TYPE_NAME_IWIDGET}, lua_init_toggle_switch},
      {STR_TYPE_NAME_PROGRESS, {STR_TYPE_NAME_IWIDGET}, lua_init_progress},
      {nullptr, {}, nullptr},
  };

  //----------------------------------------------------------------------------------

  const struct luaL_Reg LuaContext::LIB_CONTEXT[] = {
      {"manageWidget", LuaContext::lua_context_manage_widget},
      {"exit", LuaContext::lua_context_exit},
      {"getLayout", LuaContext::lua_context_get_layout},
      {nullptr, nullptr},
  };

  const struct luaL_Reg LuaContext::LIB_INPUT[] = {
      {"enable_btn", lua_input_enable_btn},
      {"disable_btn", lua_input_disable_btn},
      {"is_holded", lua_input_is_holded},
      {"is_pressed", lua_input_is_pressed},
      {"is_released", lua_input_is_released},
      {"lock", lua_input_lock},
#ifdef TOUCHSCREEN_SUPPORT
      {"getSwipe", lua_input_get_swipe},
      {"getTouchX", lua_input_get_x},
      {"getTouchY", lua_input_get_y},
#endif  // #ifdef TOUCHSCREEN_SUPPORT
      {nullptr, nullptr},
  };

  LuaContext* LuaContext::_self;

  LuaContext::LuaContext()
  {
#ifdef GRAPHICS_ENABLED
    EmptyLayout* layout = new EmptyLayout(1);
    layout->setBackColor(COLOR_BLUE);
    layout->setWidth(UI_WIDTH);
    layout->setHeight(UI_HEIGHT);
    setLayout(layout);

    _notification = new Notification(1);
#endif
    setCpuFrequencyMhz(MAX_CPU_FREQ_MHZ);
    _self = this;
  }

  LuaContext::~LuaContext()
  {
    lua_close(_lua);
    delete _notification;

    for (size_t i = 0; i < _loaded_img_id.size(); ++i)
      _res.deleteResByID(_loaded_img_id[i]);

    for (size_t i = 0; i < _managed_widgets.size(); ++i)
      delete _managed_widgets[i];

    log_i("%s", STR_LUA_STOP);
    setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
  }

  bool LuaContext::initLua()
  {
    if (_lua)
    {
      _is_script_exec = false;
      lua_close(_lua);
    }

    _lua = lua_newstate(luAlloc, nullptr);

    if (!_lua)
      return false;

    luaL_openlibs(_lua);
    lua_sethook(_lua, luaHook, LUA_MASKCOUNT, 10000);

    regRegularModule(_lua);
    regRequireModule(_lua);

    lua_register(_lua, "initType", lua_init_type);
    lua_register(_lua, "unrequire", lua_unrequire);

    lua_register(_lua, "showToast", lua_show_toast);
    lua_register(_lua, "showNotification", lua_show_notification);
    lua_register(_lua, "hideNotification", lua_hide_notification);

    lua_register(_lua, "loadImg", lua_load_img);
    lua_register(_lua, "deleteImg", lua_delete_img);

#ifdef TOUCHSCREEN_SUPPORT
    lua_pushinteger(_lua, ITouchscreen::SWIPE_NONE);
    lua_setglobal(_lua, "SWIPE_NONE");
    lua_pushinteger(_lua, ITouchscreen::SWIPE_L);
    lua_setglobal(_lua, "SWIPE_L");
    lua_pushinteger(_lua, ITouchscreen::SWIPE_R);
    lua_setglobal(_lua, "SWIPE_R");
    lua_pushinteger(_lua, ITouchscreen::SWIPE_U);
    lua_setglobal(_lua, "SWIPE_U");
    lua_pushinteger(_lua, ITouchscreen::SWIPE_D);
    lua_setglobal(_lua, "SWIPE_D");
#endif  // #ifdef TOUCHSCREEN_SUPPORT

    _msg = "";
    return true;
  }

  bool LuaContext::execScript(const char* lua_script)
  {
    if (!initLua())
      return false;

    log_i("%s", STR_LUA_START);

    if (luaL_loadstring(_lua, lua_script) != LUA_OK || lua_pcall(_lua, 0, LUA_MULTRET, 0) != LUA_OK)
    {
      luaErrToMsg();
      return false;
    }
    else if (!hasLuaFunction(STR_UPDATE_NAME))
    {
      _msg = "Скрипт повинен містити визначення функції update";
      return false;
    }

    _msg = "";
    _is_script_exec = true;
    return true;
  }

  String LuaContext::getMsg() const
  {
    return _msg;
  }

  void* LuaContext::luAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
  {
    (void)ud;
    (void)osize;

    if (nsize == 0)
    {
      free(ptr);
      return nullptr;
    }
    else
    {
      size_t free_mem = heap_caps_get_free_size(MALLOC_CAP_8BIT);

      if (nsize < free_mem && free_mem - nsize > 40 * 1024)
      {
        return realloc(ptr, nsize);
      }
      else if (psramInit())
      {
        free_mem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        if (free_mem > nsize)
          return ps_realloc(ptr, nsize);
      }

      _self->_msg = STR_NOT_ENOUGH_RAM;
      log_e("%s", STR_NOT_ENOUGH_RAM);
      return nullptr;
    }
  }

  void LuaContext::luaHook(lua_State* L, lua_Debug* ar)
  {
    (void)L;
    (void)ar;

    int used_kb = lua_gc(L, LUA_GCCOUNT, 0);
    if (used_kb > 30)
      lua_gc(L, LUA_GCCOLLECT, 0);

    if (_self->_hook_counter > 12)
    {
      _self->_hook_counter = 0;
      delay(1);
    }

    ++_self->_hook_counter;
  }

  void LuaContext::luaErrToMsg()
  {
    const char* err_msg = lua_tostring(_lua, -1);
    if (err_msg)
    {
      _msg = "Помилка Lua: ";
      _msg += err_msg;
    }
  }

  bool LuaContext::hasLuaFunction(const char* func_name)
  {
    lua_getglobal(_lua, func_name);
    bool exists = lua_isfunction(_lua, -1);
    lua_pop(_lua, 1);
    return exists;
  }

  int LuaContext::callLuaFunction(const char* func_name)
  {
    lua_getglobal(_lua, func_name);
    return lua_pcall(_lua, 0, 0, 0);
  }

  bool LuaContext::loop()
  {
    return true;
  }

  void LuaContext::update()
  {
    if (_is_script_exec)
    {
      if (callLuaFunction(STR_UPDATE_NAME) != LUA_OK)
      {
        luaErrToMsg();
        release();
      }
    }
  }

  //----------------------------------------------------------------------------------------------------

  int LuaContext::lua_register_context(lua_State* L)
  {
    luaL_newlib(L, LIB_CONTEXT);
    lua_setglobal(L, "context");
    return 0;
  }

  int LuaContext::lua_register_input(lua_State* L)
  {
    luaL_newlib(L, LIB_INPUT);
    lua_setglobal(L, "input");
    return 0;
  }

  //---------------------------------------------------------------------------------------------------- context

  int LuaContext::lua_context_exit(lua_State* L)
  {
    _self->release();
    return 0;
  }

  int LuaContext::lua_context_get_layout(lua_State* L)
  {
#ifndef GRAPHICS_ENABLED
    lua_pushnil(L);
#else   // GRAPHICS_ENABLED
    IWidgetContainer* layout = _self->getLayout();
    if (!layout)
    {
      lua_pushnil(L);
    }
    else
    {
      IWidgetContainer** udata = static_cast<IWidgetContainer**>(lua_newuserdata(L, sizeof(IWidgetContainer*)));
      *udata = layout;

      luaL_getmetatable(L, STR_TYPE_NAME_IWIDGET_CONT);
      lua_setmetatable(L, -2);
    }
#endif  // #ifdef GRAPHICS_ENABLED
    return 1;
  }

  int LuaContext::lua_context_manage_widget(lua_State* L)
  {
    IWidget** iwidget = static_cast<IWidget**>(lua_touserdata(L, 1));
    _self->_managed_widgets.push_back(*iwidget);
    return 0;
  }

  //---------------------------------------------------------------------------------------------------- input

  int LuaContext::lua_input_enable_btn(lua_State* L)
  {
    int btn = luaL_checkinteger(L, 1);
    _input.enableBtn(static_cast<BtnID>(btn));
    return 0;
  }

  int LuaContext::lua_input_disable_btn(lua_State* L)
  {
    int btn = luaL_checkinteger(L, 1);
    _input.disableBtn(static_cast<BtnID>(btn));
    return 0;
  }

  int LuaContext::lua_input_is_holded(lua_State* L)
  {
#ifdef TOUCHSCREEN_SUPPORT
    int args_num = lua_gettop(L);

    if (args_num == 0)
    {
      lua_pushboolean(L, _input.isHolded());
    }
    else
    {
#endif  // #ifdef TOUCHSCREEN_SUPPORT
      int btn = luaL_checkinteger(L, 1);
      lua_pushboolean(L, _input.isHolded(static_cast<BtnID>(btn)));
#ifdef TOUCHSCREEN_SUPPORT
    }
#endif  // #ifdef TOUCHSCREEN_SUPPORT

    return 1;
  }

  int LuaContext::lua_input_is_pressed(lua_State* L)
  {
#ifdef TOUCHSCREEN_SUPPORT
    int args_num = lua_gettop(L);

    if (args_num == 0)
    {
      lua_pushboolean(L, _input.isPressed());
    }
    else
    {
#endif  // #ifdef TOUCHSCREEN_SUPPORT
      int btn = luaL_checkinteger(L, 1);
      lua_pushboolean(L, _input.isPressed(static_cast<BtnID>(btn)));
#ifdef TOUCHSCREEN_SUPPORT
    }
#endif  // #ifdef TOUCHSCREEN_SUPPORT
    return 1;
  }

  int LuaContext::lua_input_is_released(lua_State* L)
  {
#ifdef TOUCHSCREEN_SUPPORT
    int args_num = lua_gettop(L);

    if (args_num == 0)
    {
      lua_pushboolean(L, _input.isReleased());
    }
    else
    {
#endif  // #ifdef TOUCHSCREEN_SUPPORT
      int btn = luaL_checkinteger(L, 1);
      lua_pushboolean(L, _input.isReleased(static_cast<BtnID>(btn)));
#ifdef TOUCHSCREEN_SUPPORT
    }
#endif  // #ifdef TOUCHSCREEN_SUPPORT
    return 1;
  }

  int LuaContext::lua_input_lock(lua_State* L)
  {
#ifdef TOUCHSCREEN_SUPPORT
    int args_num = lua_gettop(L);

    if (args_num == 1)
    {
      int lock_time = luaL_checkinteger(L, 1);
      _input.lock(lock_time);
    }
    else
    {
#endif  // #ifdef TOUCHSCREEN_SUPPORT
      int btn = luaL_checkinteger(L, 1);
      int lock_time = luaL_checkinteger(L, 2);
      _input.lock(static_cast<BtnID>(btn), lock_time);
#ifdef TOUCHSCREEN_SUPPORT
    }
#endif  // #ifdef TOUCHSCREEN_SUPPORT
    return 0;
  }

#ifdef TOUCHSCREEN_SUPPORT
  int LuaContext::lua_input_get_swipe(lua_State* L)
  {
    lua_pushinteger(L, _input.getSwipe());
    return 1;
  }

  int LuaContext::lua_input_get_x(lua_State* L)
  {
    lua_pushinteger(L, _input.getTouchX());
    return 1;
  }

  int LuaContext::lua_input_get_y(lua_State* L)
  {
    lua_pushinteger(L, _input.getTouchY());
    return 1;
  }
#endif  // #ifdef TOUCHSCREEN_SUPPORT

  int LuaContext::lua_show_toast(lua_State* L)
  {
    uint8_t arg_num = lua_gettop(L);
    const char* toast_str = luaL_checkstring(L, 1);

#ifdef GRAPHICS_ENABLED
    float time = TOAST_LENGTH_SHORT;

    if (arg_num > 1)
      time = luaL_checknumber(L, 2);

    _self->showToast(toast_str, time);

#else
    log_i("%s", toast_str);

#endif

    return 0;
  }

  int LuaContext::lua_show_notification(lua_State* L)
  {
    uint8_t arg_num = lua_check_top(L, {1, 4});

#ifdef GRAPHICS_ENABLED
    if (arg_num == 1)
    {
      const char* notification_msg = luaL_checkstring(L, 1);
      _self->_notification->setTitleText(STR_NOTIFICATION);
      _self->_notification->setMsgText(notification_msg);
      _self->_notification->setLeftText(emptyString);
      _self->_notification->setRightText(STR_OK);
    }
    else if (arg_num == 4)
    {
      const char* n_title = luaL_checkstring(L, 1);
      const char* n_msg = luaL_checkstring(L, 2);
      const char* n_left = luaL_checkstring(L, 3);
      const char* n_rigt = luaL_checkstring(L, 4);

      _self->_notification->setTitleText(n_title);
      _self->_notification->setMsgText(n_msg);
      _self->_notification->setLeftText(n_left);
      _self->_notification->setRightText(n_rigt);
    }
    else
    {
      return 0;
    }

    _self->showNotification(_self->_notification);

#else
    int stack_msg_pos{0};
    if (arg_num == 1)
    {
      stack_msg_pos = 1;
    }
    else if (arg_num == 4)
    {
      stack_msg_pos = 2;
    }
    else
    {
      return 0;
    }

    const char* notification_msg = luaL_checkstring(L, stack_msg_pos);
    log_i("%s", notification_msg);

#endif

    return 0;
  }

  int LuaContext::lua_hide_notification(lua_State* L)
  {
#ifdef GRAPHICS_ENABLED
    _self->hideNotification();
#endif
    return 0;
  }

  int LuaContext::lua_load_img(lua_State* L)
  {
    const char* path = luaL_checkstring(L, 1);
    uint32_t id = _res.load(path);

    if (id > 0)
      _self->_loaded_img_id.push_back(id);

    lua_pushinteger(L, id);
    return 1;
  }

  int LuaContext::lua_delete_img(lua_State* L)
  {
    uint32_t id = luaL_checkinteger(L, 1);

    for (auto it_b = _self->_loaded_img_id.begin(), it_e = _self->_loaded_img_id.end(); it_b != it_e; ++it_b)
    {
      if (*it_b == id)
      {
        _self->_loaded_img_id.erase(it_b);
        _res.deleteResByID(id);
        break;
      }
    }

    return 0;
  }

  void LuaContext::regRequireModule(lua_State* L)
  {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    int searchers_len = lua_rawlen(L, -1);
    lua_pushinteger(L, searchers_len + 1);
    lua_pushcfunction(L, registerRequire);
    lua_rawset(L, -3);
    lua_pop(L, 2);
  }

  void LuaContext::regRegularModule(lua_State* L)
  {
    const size_t funcs_num = sizeof(LIB_REGULAR_MODULE) / sizeof(LuaRegisterFunc);
    for (size_t i = 0; i < funcs_num; ++i)
      LIB_REGULAR_MODULE[i](L);
  }

  bool LuaContext::initType(lua_State* L, const char* type_name)
  {
    if (isTypeInited(type_name))
      return true;

    for (const auto& type : LIB_CUSTOM_TYPE)
    {
      if (type.name && strcmp(type.name, type_name) == 0)
      {
        for (const char* dep_name : type.deps)
        {
          if (!initType(L, dep_name))
            return false;  // Неможливо ініціалізувати одну із залежностей
        }

        if (type.initializer)
        {
          type.initializer(L);                 // Реєструємо основний тип
          _inited_types.push_back(type_name);  // Запам'ятали реєстрацію
          return true;
        }
      }
    }

    luaL_error(L, "Відсутній ініціалізатор для типу: %s", type_name);
    return false;
  }

  void LuaContext::clearInitialization(const char* type_name)
  {
    for (auto it_b = _inited_types.begin(), it_e = _inited_types.end(); it_b != it_e; ++it_b)
    {
      if (strcmp(*it_b, type_name) == 0)
      {
        _inited_types.erase(it_b);
        return;
      }
    }
  }

  int LuaContext::lua_init_type(lua_State* L)
  {
    const char* type_name = luaL_checkstring(L, 1);
    _self->initType(L, type_name);
    return 0;
  }

  int LuaContext::lua_unrequire(lua_State* L)
  {
    const char* module_name = luaL_checkstring(L, 1);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_getfield(L, -1, module_name);  // stack: package, loaded, module

    if (!lua_isnil(L, -1))
    {
      lua_getfield(L, -1, "__unload");
      if (!lua_isfunction(L, -1))
      {
        lua_pop(L, 1);
      }
      else
      {
        lua_pushvalue(L, -2);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
          const char* err = lua_tostring(L, -1);
          log_e("Error in __unload: %s\n", err);
          lua_pop(L, 1);
        }
      }
    }

    lua_pop(L, 1);

    lua_pushnil(L);
    lua_setfield(L, -2, module_name);

    lua_pop(L, 2);

    lua_pushnil(L);
    lua_setglobal(L, module_name);

    _self->clearInitialization(module_name);  // Забули реєстрацію

    return 0;
  }

  bool LuaContext::isTypeInited(const char* type_name)
  {
    for (const char* t_name : _inited_types)
    {
      if (strcmp(t_name, type_name) == 0)
        return true;
    }

    return false;
  }

  //----------------------------------------------------------------------------------

}  // namespace pixeler