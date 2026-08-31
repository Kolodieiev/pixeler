#pragma once

#include "IChessGameContext.h"
#include "game/online/GameServer.h"

namespace chess
{
  class ChessServerContext : public IChessGameContext
  {
  public:
    ChessServerContext();
    virtual ~ChessServerContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    void showLobbyTmpl();     // Показати ігрове лобі сервера
    void handleLobbyInput();  // Обробка клавіш ігрового лобі

    void showLobbyContextMenuTmpl();  // Показати контекстне меню ігрового лобі сервера
    void hideLobbyContextMenu();      // Приховати контекстне меню ігрового лобі сервера
    void handleContextMenuInput();    // Обробка клавіш контекстного меню ігрового лобі

    void showClientConfirmTmpl(String client_name);  // Показати повідомлення про підключення клієнта
    void handleClientConfirmInput();                 // Прийняти або відхилити клієнта
    void handleClientConfirmResult(bool is_accepted);

    static void onConfirmationHandler(const String client_name, void* arg);  // Обробник запиту на підключення
    static void onDisconnectHandler(const String client_name, void* arg);    // Обробник відключення клієнта

  private:
    using StateHandler = void (ChessServerContext::*)();

    enum WidgetID : uint8_t
    {
      ID_CLIENT_LIST = 1,
      ID_LBL_CONFIRM_TITLE,
      ID_LBL_CLIENT_NAME,
      ID_LBL_CONFIRM_WAY,
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

    StateHandler _state_input_handler{nullptr};
  };
}  // namespace chess
