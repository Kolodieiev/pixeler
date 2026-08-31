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
     * @brief Перечислення, що містить значення станів клієнта.
     *
     */
    enum ClientStatus : uint8_t
    {
      STATUS_IDLE = 0,          // В очікуванні.
      STATUS_CONNECTED,         // Приєднано до сервера.
      STATUS_DISCONNECTED,      // З'єднання з сервером втрачено.
      STATUS_INCORRECT_SERVER,  // Некоректний сервер.
      STATUS_INCORRECT_NAME,    // Некоректний нікнейм.
      STATUS_ACCESS_DENIED,     // Відмовлено в авторизації.
      STATUS_SERVER_BUSY,       // Сервер зайнятий обробкою інших запитів.
    };

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
     * @brief Надсилає пакет на сервер, з яким встановлено з'єднання.
     *
     * @param packet Пакет, що буде надіслано на сервер
     */
    void sendPacket(const UdpPacket& packet);

    /**
     * @brief Формує та надсилає пакет на сервер, з яким встановлено з'єднання.
     *
     * @param type Тип пакета
     * @param data Дані пакета
     * @param data_size Розмір даних
     */
    void send(UdpPacket::PacketType type, const void* data, size_t data_size);

    /**
     * @brief Повертає поточний статус клієнта.
     *
     * @return ClientStatus
     */
    ClientStatus getStatus() const;

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета даних від сервера.
     *
     * @param handler Обробник події отримання даних
     * @param arg Аргумент, який будуе передано обробнику
     */
    void onData(DataHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після встановлення з'єднання з сервером.
     *
     * @param handler Обробник події встановлення з'єднання з сервером
     * @param arg Аргумент, який буде передано обробнику
     */
    void onConnect(ConnectHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після втрати з'єднання з сервером.
     *
     * @param handler Обробник події втрати з'єднання з сервером
     * @param arg Аргумент, який будуе передано обробнику
     */
    void onDisconnect(DisconnectHandler handler, void* arg);

  protected:
    void sendHandshake();
    void sendName();
    //
    void handlePacket(const UdpPacket& packet);
    static void packetHandlerTask(void* arg);
    //
    static void onPacket(void* arg, AsyncUDPPacket& packet);
    //
    void handleCheckConnect();
    static void checkConnectTask(void* arg);
    //
    void handleHandshake(const UdpPacket& packet);
    void handleConfirmResult(const UdpPacket& packet);
    void handleIncorrectName();
    void handlePing();
    void handleBusy();
    //
    void invokeDataHandler(const UdpPacket& packet);
    void invokeConnectHandler();
    void invokeDisconnectHandler();

  protected:
    AsyncUDP _client;
    IPAddress _server_ip;

    ConnectHandler _connect_handler{nullptr};
    DisconnectHandler _disconnect_handler{nullptr};
    DataHandler _data_handler{nullptr};

    String _nickname;
    String _game_id;

    SemaphoreHandle_t _udp_mutex{nullptr};

    QueueHandle_t _packet_queue;

    TaskHandle_t _check_task_handler{nullptr};
    TaskHandle_t _packet_task_handler{nullptr};

    void* _connect_arg{nullptr};
    void* _disconnect_arg{nullptr};
    void* _data_arg{nullptr};

    unsigned long _last_act_time{0};

    ClientStatus _status{STATUS_DISCONNECTED};

    bool _is_freed{true};
  };
}  // namespace pixeler
