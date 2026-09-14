#pragma once
#pragma GCC optimize("O3")
#include <AsyncUDP.h>

#include "UdpPacket.h"
#include "defines.h"

namespace pixeler
{
  class GameClient
  {
  public:
    /**
     * @brief Перечислення, що містить значення станів клієнта.
     *
     */
    enum Status : uint8_t
    {
      STATUS_IDLE = 0,      // В очікуванні.
      STATUS_CONNECTED,     // Приєднано до сервера.
      STATUS_DISCONNECTED,  // З'єднання з сервером втрачено.
    };

    /**
     * @brief Перечислення, що містить значення помилок клієнта.
     *
     */
    enum Error : uint8_t
    {
      ERR_INCORRECT_SERVER = 0,  // Некоректний сервер.
      ERR_INCORRECT_NAME,        // Некоректний нікнейм.
      ERR_ACCESS_DENIED,         // Відмовлено в авторизації.
      ERR_SERVER_BUSY,           // Сервер зайнятий обробкою інших запитів.
    };

    /**
     * @brief Тип обробника, який може бути викликано клієнтом у разі втрати зв'язку з сервером.
     *
     */
    using DisconnectHandler = std::function<void(void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом після встановлення зв'язку з сервером.
     *
     */
    using ConnectHandler = std::function<void(void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом після отримання пакета даних від сервера.
     *
     */
    using DataHandler = std::function<void(const UdpPacket& packet, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом після отримання повідомлення про помилку.
     *
     */
    using ErrorHandler = std::function<void(GameClient::Error error, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом у разі отримання пакета початку гри.
     *
     */
    using GameStartHandler = std::function<void(void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом у разі отримання пакета завершення гри.
     *
     */
    using GameStopHandler = std::function<void(void* arg)>;

    GameClient();
    ~GameClient();

    /**
     * @brief Запускає процедуру підключення до сервера.
     *
     * @param client_name Ім'я клієнта
     * @param _game_ID Ідентифікатор ігрового сервера
     * @param host_IP IP-адреса сервера
     * @return true - Якщо процедуру підключення запущено успішно. false - інакше
     */
    bool connect(const String& client_name, const String& game_ID, const String& host_IP = "192.168.4.1");

    /**
     * @brief Скидає всі обробники, від'єднує від сервера та звільняє зайняті ресурси.
     * Модуль WiFi не вимикається.
     *
     */
    void disconnect();

    /**
     * @brief Формує та надсилає пакет на сервер, з яким встановлено з'єднання.
     *
     * @param type Тип пакета
     * @param subtype Підтип основго типу пакета
     * @param data_size Розмір даних
     * @param data Дані пакета
     */
    void send(UdpPacket::PacketType type, uint8_t subtype, size_t data_size = 0, const void* data = nullptr);

    /**
     * @brief Повертає поточний статус клієнта.
     *
     * @return Status
     */
    Status getStatus() const;

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета даних від сервера.
     *
     * @param handler Обробник події отримання даних
     * @param arg Аргумент, який буде передано обробнику
     */
    void onGameData(DataHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета з командою початку гри.
     *
     * @param handler Обробник події отримання пакета початку гри
     * @param arg Аргумент, який буде передано обробнику
     */
    void onGameStart(GameStartHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета з командою зупинки гри.
     *
     * @param handler Обробник події отримання пакета зупинки гри
     * @param arg Аргумент, який буде передано обробнику
     */
    void onGameStop(GameStopHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після встановлення з'єднання з сервером.
     *
     * @param handler Обробник події встановлення з'єднання з сервером
     * @param arg Аргумент, який буде передано обробнику
     */
    void onConnect(ConnectHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після отримання повідомлення про помилку.
     *
     * @param handler Обробник події отримання помилки
     * @param arg Аргумент, який буде передано обробнику
     */
    void OnError(ErrorHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після втрати з'єднання з сервером.
     *
     * @param handler Обробник події втрати з'єднання з сервером
     * @param arg Аргумент, який буде передано обробнику
     */
    void onDisconnect(DisconnectHandler handler, void* arg);

  protected:
    void send(const UdpPacket& packet);
    void sendAck(uint8_t packet_id = 0);
    //
    void sendHandshake();
    void sendLogin();
    //
    void handlePacket(const UdpPacket& packet);
    static void packetHandlerTask(void* arg);
    //
    static void onPacket(void* arg, AsyncUDPPacket& packet);
    //
    void handleCheckConnect();
    static void checkConnectTask(void* arg);
    //
    void handleConnect(const UdpPacket& packet);
    void handlePing();
    void handleServiceData(const UdpPacket& packet);
    //
    void invokeDataHandler(const UdpPacket& packet);
    void invokeConnectHandler();
    void invokeDisconnectHandler();
    void invokeErrorHandler(Error error);

    void handleGameStart(const UdpPacket& packet);
    void handleGameStop(const UdpPacket& packet);

  protected:
    AsyncUDP _client;
    IPAddress _server_ip;

    ConnectHandler _connect_handler{nullptr};
    DisconnectHandler _disconnect_handler{nullptr};
    DataHandler _data_handler{nullptr};
    ErrorHandler _error_handler{nullptr};
    GameStartHandler _start_handler{nullptr};
    GameStopHandler _stop_handler{nullptr};

    String _login;
    String _game_id;

    mutable SemaphoreHandle_t _udp_mutex{nullptr};

    QueueHandle_t _packet_queue;

    TaskHandle_t _check_task_handler{nullptr};
    TaskHandle_t _packet_task_handler{nullptr};

    void* _connect_arg{nullptr};
    void* _disconnect_arg{nullptr};
    void* _data_arg{nullptr};
    void* _error_arg{nullptr};
    void* _start_arg{nullptr};
    void* _stop_arg{nullptr};

    unsigned long _last_act_time{0};

    Status _status{STATUS_DISCONNECTED};

    int16_t _last_packet_id{-1};

    bool _is_freed{true};
    bool _wifi_was_enabled{false};
  };
}  // namespace pixeler
