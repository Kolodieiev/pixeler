#pragma once

#include "IChessGameContext.h"
#include "game/online/GameClient.h"
#include "manager/WiFiManager.h"

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
    void setupWiFi();
    void showAPScanTmpl();     // Показати сканування точок доступа для клієнта
    void handleAPScanInput();  // Обробка відміни сканування

    void showAPListTmpl();     // Показати список точок доступа для клієнта
    void handleAPListInput();  // Вибір точки доступу та підключення

    void showAPConnectTmpl();     // Показати підключення до точки доступу
    void handleAPConnectInput();  // Обробка відміни підключення до AP

    void showLobbyTmpl();     // Показати клієнтське лобі
    void handleLobbyInput();  // Вихід з клієнтського лобі

    void showConnectDialogTmpl();     // Показати діалог підключення до AP
    void handleConnectDialogInput();  // Обробка вводу діалогово вікна

    static void onConnectHandler(void* arg);     // Обробник події підключення до сервера
    static void onDisconnectHandler(void* arg);  // Обробник події відключення від сервера

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

    pixeler::GameClient _client;

    StateHandler _state_input_handler{nullptr};
  };
}  // namespace chess
