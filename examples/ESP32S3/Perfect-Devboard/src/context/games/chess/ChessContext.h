#pragma once
//
#include "game/DataStream.h"
//
#include "context/IContext.h"
#include "game/2D/IGameScene2D.h"
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
    using StateHandler = void (ChessContext::*)();

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

    enum PrefDialogID : uint8_t
    {
      DIALOG_ID_NICK = 0,
      DIALOG_ID_SERV_NAME,
      DIALOG_ID_SERV_PWD,
      DIALOG_ID_SERVER_CONN,
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
    void procMainMenu();  // Обробка вводу головного меню

    // Клієнт

    void showWifiScanTmpl();  // Показати сканування точок доступа для клієнта
    void procWifiScan();      // Обробка відміни сканування

    void showWifiListTmpl();  // Показати список точок доступа для клієнта
    void procWifiList();      // Вибір точки доступу та підключення

    void showConnToApTmpl();  // Показати підключення до точки доступу
    void procConnectToAp();   // Обробка відміни підключення до AP

    void showClientLobbyTmpl();  // Показати клієнтське лобі
    void procClientLobby();      // Вихід з клієнтського лобі

    // Сервер

    void showClientConfirmTmpl();   // Показати повідомлення про підключення клієнта
    void procClientConfirmation();  // Прийняти або відхилити клієнта

    void showServerContextTmpl();  // Показати контекстне меню сервера
    void procLobbyContext();       // Обробка серверного меню

    void showServerLobbyTmpl();  // Показати ігрове лобі сервера
    void procServerLobby();      // Вибір клієнта зі списку

    // Налаштування

    void showPrefsTmpl();  // Показати головне вікно налаштувань
    void procPrefMenu();   // Вибір пункту налаштувань

    void showConnDialogTmpl();                                         // Показати діалог підключення до AP
    void showDialogNicknameTmpl();                                     // Показати вікно налаштувань нікнейма
    void showDialogServNameTmpl();                                     // Показати вікно налаштувань серверного імені
    void showDialogServPwdTmpl();                                      // Показати вікно налаштувань серверного пароля
    void addDialog(const String& title_txt, const String& start_txt);  // Додати до layout діалог вводу
    void procDialog();                                                 // Обробка вводу діалогового вікна

    void updateGame();

    //---------------------------------------------------------------

  private:
    String _client_nick;
    String _serv_ssid;
    String _serv_pwd;

    DataStream _stored_objs{10};                               // Об'єкт для перенесення даних між сценами
    StateHandler _current_state{&ChessContext::procMainMenu};  // Змінна для збереження методу, який відповідає за обробку поточного стану контекста
    IGameScene2D* _scene{nullptr};

    PrefDialogID _dialog_id{DIALOG_ID_NICK};
  };
}  // namespace chess
