#pragma once

#include "IChessGameContext.h"
#include "game/online/GameServer.h"

namespace chess
{
  class ServerChessContext : public IChessGameContext
  {
  public:
    ServerChessContext();
    virtual ~ServerChessContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    void setupServer();  // Налаштувати ігровий сервер

    void showLobbyTmpl();     // Показати ігрове лобі сервера
    void handleLobbyInput();  // Обробка клавіш ігрового лобі

    void showLobbyContextMenuTmpl();  // Показати контекстне меню ігрового лобі сервера
    void hideLobbyContextMenu();      // Приховати контекстне меню ігрового лобі сервера
    void handleContextMenuInput();    // Обробка клавіш контекстного меню ігрового лобі
    void scrollClientsMenu(bool scroll_up = false);

    void showClientAcceptTmpl(String client_name);  // Показати повідомлення про підключення клієнта
    void handleClientAcceptInput();                 // Прийняти або відхилити клієнта
    void handleClientAcceptResult(bool is_accepted);

    static void onAcceptHandler(const String client_name, void* arg);        // Обробник запиту на підключення
    static void onDisconnectHandler(const IPAddress& client_ip, void* arg);  // Обробник відключення клієнта

    void unsubscribeServerHandlers();
    void subscribeServerHandlers();
    void startGame(const String& main_client_name);
    void handleGame();

  private:
    using StateHandler = void (ServerChessContext::*)();

    enum WidgetID : uint8_t
    {
      ID_CLIENT_LIST = 1,
      ID_LBL_ACCEPT_TITLE,
      ID_LBL_CLIENT_NAME,
      ID_LBL_ACCEPT_WAY,
      ID_LBL_REJECT_WAY,
      ID_EMPTY_MSG,
      ID_CONTEXT_MENU,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_TOGGLE_LOBBY = 1,
      ID_ITEM_START_GAME,
      ID_ITEM_KICK_CLIENT,
    };

    pixeler::GameServer _server;

    StateHandler _state_handler{nullptr};

    bool _wifi_was_enabled{false};
  };
}  // namespace chess
