/**
 * @file LuaContext.h
 * @brief Клас контексту для керування станом Lua-машини
 * @details Запускає/зупиняє віртуальну Lua-машину, виділяє/звільняє ресурси для її функціонування.
 * Реєструє типи для Lua.
 * Дозволяє передати скрипт на виконання віртуальній машині Lua.
 * Повертає повідомлення від Lua, якщо виникла помилка виконання.
 */

#pragma once
#pragma GCC optimize("O3")

#include "../lua.h"
#include "context/IContext.h"

namespace pixeler
{
  class LuaContext : public IContext
  {
  public:
    LuaContext();
    virtual ~LuaContext();

    /**
     * @brief Завантажує скрипт до віртуальної машини Lua.
     *
     * @param lua_script Рядок, що містить скрипт.
     * @return true - Якщо скрипт коректно завантажено до Lua-машини.
     * @return false - Інакше.
     */
    bool execScript(const char* lua_script);

    /**
     * @brief Повертає повідомлення від Lua, якщо виникла помилка виконання скрипта.
     *
     * @return String
     */
    String getMsg() const;

  protected:
    virtual bool loop() override;
    virtual void update() override;
    //
  private:
    typedef void (*InitLuaTypeFunc)(lua_State* L);

    typedef struct LuaCustomType
    {
      std::vector<const char*> deps;
      const char* name;
      InitLuaTypeFunc initializer;

      /**
       * @brief Конструює новий об'єкт LuaCustomType.
       *
       * @param n Ім'я типу в Lua.
       * @param d Список залежностей (Імен інших типів).
       * @param f Функція-ініціалізатор типу.
       */
      LuaCustomType(const char* n, std::vector<const char*> d, InitLuaTypeFunc f) : name(n), deps(d), initializer(f) {}
    } LuaCustomType;

    static void* luAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
    static void luaHook(lua_State* L, lua_Debug* ar);
    //
    bool initLua();
    void luaErrToMsg();
    bool hasLuaFunction(const char* func_name);
    int callLuaFunction(const char* func_name);

    //---------------------------------------------------------------------------------- reg

    static int lua_register_context(lua_State* L);
    static int lua_register_input(lua_State* L);

    //---------------------------------------------------------------------------------- context

    static int lua_context_exit(lua_State* L);
    static int lua_context_get_layout(lua_State* L);
    static int lua_context_manage_widget(lua_State* L);

    //---------------------------------------------------------------------------------- input

    static int lua_input_enable_btn(lua_State* L);
    static int lua_input_disable_btn(lua_State* L);
    static int lua_input_is_holded(lua_State* L);
    static int lua_input_is_pressed(lua_State* L);
    static int lua_input_is_released(lua_State* L);
    static int lua_input_lock(lua_State* L);

#ifdef TOUCHSCREEN_SUPPORT
    static int lua_input_get_swipe(lua_State* L);
    static int lua_input_get_x(lua_State* L);
    static int lua_input_get_y(lua_State* L);
#endif  // #ifdef TOUCHSCREEN_SUPPORT
    //---------------------------------------------------------------------------------- Helper

    static int lua_show_toast(lua_State* L);
    static int lua_show_notification(lua_State* L);
    static int lua_hide_notification(lua_State* L);
    static int lua_load_img(lua_State* L);
    static int lua_delete_img(lua_State* L);

    //---------------------------------------------------------------------------------- Require

    void regRequireModule(lua_State* L);
    static int registerRequire(lua_State* L);

    //---------------------------------------------------------------------------------- Regular

    void regRegularModule(lua_State* L);

    //---------------------------------------------------------------------------------- Custom type
    static int lua_init_type(lua_State* L);
    static int lua_unrequire(lua_State* L);

    bool isTypeInited(const char* type_name);
    bool initType(lua_State* L, const char* type_name);
    void clearInitialization(const char* type_name);
    //----------------------------------------------------------------------------------

  private:
    String _msg;
    std::vector<const char*> _inited_types;
    std::vector<uint32_t> _loaded_img_id;
    std::vector<IWidget*> _managed_widgets;

    static LuaContext* _self;
    static const LuaRegisterFunc LIB_REGULAR_MODULE[];
    static const LuaCustomType LIB_CUSTOM_TYPE[];
    static const struct luaL_Reg LIB_CONTEXT[];
    static const struct luaL_Reg LIB_INPUT[];

    lua_State* _lua{nullptr};
    Notification* _notification{nullptr};

    uint16_t _hook_counter{0};

    bool _is_script_exec = false;
  };
}  // namespace pixeler