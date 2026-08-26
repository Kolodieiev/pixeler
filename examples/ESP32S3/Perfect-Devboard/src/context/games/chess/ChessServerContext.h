#pragma once

#include "IChessGameContext.h"

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
    void showServerLobbyTmpl();  // Показати ігрове лобі сервера
    void procServerLobby();      // Вибір клієнта зі списку

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

    StateHandler _current_state{nullptr};
  };
}  // namespace chess
