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
    void showLobbyTmpl();  // Показати ігрове лобі сервера
    void procLobbyKb();    // Обробка клавіш ігрового лобі

    void showContextMenuTmpl();  // Показати контекстне меню ігрового лобі сервера
    void procContextMenuKb();    // Обробка клавіш контекстного меню ігрового лобі

    void showClientConfirmTmpl();   // Показати повідомлення про підключення клієнта
    void procClientConfirmation();  // Прийняти або відхилити клієнта

    void showServerContextTmpl();  // Показати контекстне меню сервера
    void procLobbyContext();       // Обробка серверного меню

  private:
    using StateHandler = void (ChessServerContext::*)();

    enum WidgetID : uint8_t
    {
      ID_MAIN_MENU = 1,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_NICK = 1,
    };

    pixeler::GameServer _server;

    StateHandler _state_kb_handler{nullptr};
  };
}  // namespace chess
