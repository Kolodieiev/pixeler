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
    using ServerDisconnectHandler = std::function<void(void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом після встановлення зв'язку з сервером.
     *
     */
    using ServerConnectHandler = std::function<void(void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано клієнтом після отримання пакета даних від сервера.
     * Об'єкт UdpPacket не потрібно видаляти самостійно.
     *
     */
    using ServerDataHandler = std::function<void(UdpPacket* packet, void* arg)>;

    /**
     * @brief Перечислення, що містить значення станів клієнта.
     *
     */
    enum ClientStatus : uint8_t
    {
      STATUS_IDLE = 0,      // В очікуванні.
      STATUS_CONNECTED,     // Приєднано до сервера.
      STATUS_DISCONNECTED,  // З'єднання з сервером втрачено.
      STATUS_WRONG_SERVER,  // Некоректний сервер.
      STATUS_WRONG_NAME,    // Відмовлено в авторизації.
      STATUS_SERVER_BUSY,   // Сервер зайнятий обробкою інших запитів.
    };

    GameClient();
    ~GameClient();

    /**
     * @brief Встановлює ім'я клієнта.
     *
     * @param name Ім'я клієнта.
     */
    void setName(const char* name);

    /**
     * @brief Повертає вказівник на поточне ім'я клієнта.
     *
     * @return const char*
     */
    const char* getName() const;

    /**
     * @brief Встановлює ідентифікатор сервера, до якого очікується підключення.
     *
     * @param id Ідентифікатор сервера.
     */
    void setServerID(const char* id);

    /**
     * @brief Запускає процедуру підключення до сервера.
     *
     * @param host_ip IP-адреса сервера.
     * @return true - Якщо процедуру підключення запущено успішно.
     * @return false - Інакше.
     */
    bool connect(const char* host_ip = "192.168.4.1");

    /**
     * @brief Скидає всі обробники, від'єднує від сервера та звільняє зайняті ресурси.
     * Модуль WiFi не вимикається.
     *
     */
    void disconnect();

    /**
     * @brief Надсилає пакет на сервер, з яким встановлено з'єднання.
     *
     * @param packet Пакет, що буде надіслано на сервер.
     */
    void sendPacket(const UdpPacket& packet);

    /**
     * @brief Формує та надсилає пакет на сервер, з яким встановлено з'єднання.
     *
     * @param type Тип пакету.
     * @param data Дані пакету.
     * @param data_size Розмір даних.
     */
    void send(UdpPacket::PacketType type, const void* data, size_t data_size);

    /**
     * @brief Повертає поточний статус клієнта.
     *
     * @return ClientStatus
     */
    ClientStatus getStatus() const;

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакету даних від сервера.
     *
     * @param data_handler Обробник події отримання даних.
     * @param arg Аргумент, який будуе передано обробнику.
     */
    void onData(const ServerDataHandler data_handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після встановлення з'єднання з сервером.
     *
     * @param conn_handler Обробник події встановлення з'єднання з сервером.
     * @param arg Аргумент, який буде передано обробнику.
     */
    void onConnect(const ServerConnectHandler conn_handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після втрати з'єднання з сервером.
     *
     * @param disconn_handler Обробник події втрати з'єднання з сервером.
     * @param arg Аргумент, який будуе передано обробнику.
     */
    void onDisconnect(ServerDisconnectHandler disconn_handler, void* arg);

  protected:
    void sendHandshake();
    void sendName();
    //
    void handlePacket(UdpPacket* packet);
    static void packetHandlerTask(void* arg);
    //
    static void onPacket(void* arg, AsyncUDPPacket& packet);
    //
    void handleCheckConnect();
    static void checkConnectTask(void* arg);
    //
    void handleHandshake(const UdpPacket* packet);
    void handleNameConfirm(const UdpPacket* packet);
    void handlePing();
    void handleBusy();
    //
    void invokeDataHandler(UdpPacket* packet);
    void invokeConnectHandler();
    void invokeDisconnHandler();

  protected:
    AsyncUDP _client;

    IPAddress _server_ip;

    ServerConnectHandler _server_connected_handler{nullptr};
    ServerDisconnectHandler _server_disconn_handler{nullptr};
    ServerDataHandler _server_data_handler{nullptr};

    String _name;
    String _server_id;

    SemaphoreHandle_t _udp_mutex{nullptr};

    QueueHandle_t _packet_queue;

    TaskHandle_t _check_task_handler{nullptr};
    TaskHandle_t _packet_task_handler{nullptr};

    void* _server_connected_arg{nullptr};
    void* _server_disconn_arg{nullptr};
    void* _server_data_arg{nullptr};

    unsigned long _last_act_time{0};

    ClientStatus _status{STATUS_DISCONNECTED};

    bool _is_freed{true};
  };
}  // namespace pixeler
