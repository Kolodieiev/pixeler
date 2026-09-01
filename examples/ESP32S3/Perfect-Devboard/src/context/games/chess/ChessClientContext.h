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
    void updateGame();

    void showStateLabelTmpl(const String& msg_str);  // Метод для відображення повідомлення про поточну дію

    void startScanAP();        // Метод для запуску сканування точок доступу
    void showAPScanTmpl();     // Показати сканування точок доступа для клієнта
    void handleAPScanInput();  // Обробка відміни сканування

    void showAPListTmpl();                         // Показати список точок доступа для клієнта
    void handleAPListInput();                      // Вибір точки доступу та підключення
    void scrollSSIDsMenu(bool scroll_up = false);  // Метод для прокрутки списку знайдених точок доступу

    void showPwdDialogTmpl(const String& ssid);  // Показати діалог вводу пароля до точки доступу
    void handlePwdDialogInput();                 // Обробити ввід пароля

    void connectToAP();               // Метод для обробки підключення до точки доступу
    void showApConnectTmpl();         // Показати підключення до точки доступу
    void connectToServer();           // Метод для обробки підключення до серверу
    void showClientConnectTmpl();     // Показати підключення до сервера
    void handleClientConnectInput();  // Обробка відміни підключення до AP
    void cancelClientConnect();       // Вихід з лоббі або процесу підключення до сервера
    void showLobbyTmpl();             // Показати клієнтське лобі
    void handleLobbyInput();          // Вихід з клієнтського лобі

    static void onClientConnectHandler(void* arg);                                  // Обробник події підключення до сервера
    static void onClientDisconnectHandler(void* arg);                               // Обробник події відключення від сервера
    static void onClientDataHandler(const pixeler::UdpPacket& packet, void* arg);   // Обробник події отримання даних від сервера
    static void onClientErrorHandler(pixeler::GameClient::Error error, void* arg);  // Обробник події отримання помилки клієнта

    static void onScanCompleteHandler(void* arg);                                       // Обробник події завешення сканування точок доступу
    static void onApConnectHandler(void* arg, const String& ssid, wl_status_t status);  // Обробник події заверешення спроби підключення до точки доступу

  private:
    using StateHandler = void (ChessClientContext::*)();

    enum WidgetID : uint8_t
    {
      ID_STATE_LBL = 1,
      ID_SSIDS_LIST,
      ID_DIALOG_LBL,
      ID_DIALOG_TEXT,
      ID_DIALOG_KB,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_NICK = 1,
    };

    pixeler::GameClient _client;

    StateHandler _state_input_handler{nullptr};

    bool _wifi_was_enabled{false};
  };
}  // namespace chess
