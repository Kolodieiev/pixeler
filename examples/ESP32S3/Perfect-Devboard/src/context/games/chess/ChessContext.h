#pragma once
//
#include "game/DataStream.h"
//
#include "context/IContext.h"
#include "game/IGameScene.h"
#include "widget/menu/FixedMenu.h"

using namespace pixeler;

namespace chess
{
  class ChessContext : public IContext
  {
  public:
    ChessContext();
    virtual ~ChessContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    enum Widget_ID : uint8_t
    {
      ID_MAIN_MENU = 1,
      ID_DIALOG_LBL,
      ID_DIALOG_TEXT,
      ID_DIALOG_KB,
    };

    enum Item_ID : uint8_t
    {
      ID_ITEM_ONE_PLAYER = 1,
      ID_ITEM_TWO_PLAYERS,
      ID_ITEM_CLIENT,
      ID_ITEM_SERVER,
      ID_ITEM_PREFS,
      ID_ITEM_NICK,
      ID_ITEM_SERV_NAME,
      ID_ITEM_SERV_PWD
    };

    enum Mode : uint8_t
    {
      MODE_MAIN = 1,  // Головне меню
      MODE_GAME,      // Режим гри
      //
      MODE_WIFI_SCAN,     // Сканування точок доступу
      MODE_WIFI_LIST,     // Відображення результату сканування
      MODE_CONN_DIALOG,   // Діалог підключення до точки доступу
      MODE_CONN_TO_AP,    // Вікно підключення до точки доступу
      MODE_CLIENT_LOBBY,  // Клієнтське лобі
      //
      MODE_SERVER_LOBBY,     // Серверне лобі
      MODE_LOBBY_CTXT_MENU,  // Контекстне меню серверного лобі
      MODE_CLIENT_CONFIRM,   // Діалог підключення клієнта
      //
      MODE_PREF_MAIN,       // Головне вікно налаштувань
      MODE_PREF_NICK,       // Діалог ігрового імені
      MODE_PREF_SERV_NAME,  // Діалог серверного імені
      MODE_PREF_SERV_PWD,   // Даілог серверного пароля
    };

    enum GameMode : uint8_t
    {
      GAME_MODE_ONE_PL = 1,
      GAME_MODE_TWO_PL,
      GAME_MODE_CLIENT,
      GAME_MODE_SERVER,
    };
    //---------------------------------------------------------------
    void loadPrefs();  // Завантаження налаштувань гри
    //---------------------------------------------------------------

    void startGame(GameMode mode);  // Створення ігрової сцени

    //---------------------------------------------------------------

    void showMainTmpl();  // Головне меню

    // Клієнт

    void showWifiScanTmpl();     // Показати сканування точок доступа для клієнта
    void showWifiListTmpl();     // Показати список точок доступа для клієнта
    void showConnDialogTmpl();   // Показати діалог підключення до AP
    void showConnToApTmpl();     // Показати підключення до точки доступу
    void showClientLobbyTmpl();  // Показати клієнтське лобі

    // Сервер

    void showNewClientConnTmpl();  // Показати повідомлення про підключення клієнта
    void showServerCtxTmpl();      // Показати контекстне меню сервера
    void showServerLobbyTmpl();    // Показати ігрове лобі сервера

    // Налаштування

    void showPrefsTmpl();                                              // Показати головне вікно налаштувань
    void showPrefsNickTmpl();                                          // Показати вікно налаштувань нікнейма
    void showPrefsServNameTmpl();                                      // Показати вікно налаштувань серверного імені
    void showPrefsServPwdTmpl();                                       // Показати вікно налаштувань серверного пароля
    void addDialog(const String& title_txt, const String& start_txt);  // Додати до layout діалог вводу
    //---------------------------------------------------------------

    void updateGame();
    //
    void procKbMain();         // Вибір режиму(головне меню)
    void procKbWifiScan();     // Обробка відміни сканування
    void procKbWifiList();     // Вибір точки доступу та підключення
    void procKbConnToAp();     // Обробка відміни підключення до AP
    void procKbClientLobby();  // Вихід з клієнтського лобі
    //
    void procKbClientConfirm();  // Прийняти або відхилити клієнта
    void procKbCtxtLobby();      // Обробка серверного меню
    void procKbServerLobby();    // Вибір клієнта зі списку
    //
    void procKbPrefMain();    // Вибір пункту налаштувань
    void procKbPrefDialog();  // Обробка вводу діалогового вікна

    //---------------------------------------------------------------

  private:
    String _client_nick;
    String _serv_ssid;
    String _serv_pwd;

    DataStream _stored_objs{10};  // Об'єкт для перенесення даних між сценами
    IGameScene* _scene{nullptr};

    Mode _mode{MODE_MAIN};  // Поточний режим контекста(стан)
  };
}  // namespace chess
