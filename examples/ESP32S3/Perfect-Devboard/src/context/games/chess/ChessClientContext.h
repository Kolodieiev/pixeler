#pragma once

#include "IChessGameContext.h"

namespace chess
{
  class ChessClientContext : public IChessGameContext
  {
  public:
    ChessClientContext();
    virtual ~ChessClientContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    void showWifiScanTmpl();  // Показати сканування точок доступа для клієнта
    void procWifiScan();      // Обробка відміни сканування

    void showWifiListTmpl();  // Показати список точок доступа для клієнта
    void procWifiList();      // Вибір точки доступу та підключення

    void showConnToApTmpl();  // Показати підключення до точки доступу
    void procConnectToAp();   // Обробка відміни підключення до AP

    void showClientLobbyTmpl();  // Показати клієнтське лобі
    void procClientLobby();      // Вихід з клієнтського лобі

    void showConnDialogTmpl();  // Показати діалог підключення до AP

  private:
    using StateHandler = void (ChessClientContext::*)();

    enum WidgetID : uint8_t
    {
      ID_MAIN_MENU = 1,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_NICK = 1,
    };

    StateHandler _state_input_handler{nullptr};
  };
}  // namespace chess
