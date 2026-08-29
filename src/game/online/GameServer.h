#pragma once
#pragma GCC optimize("O3")
#include <AsyncUDP.h>

#include <unordered_map>

#include "ClientSession.h"
#include "UdpPacket.h"
#include "defines.h"

namespace pixeler
{
  class GameServer
  {
  public:
    /**
     * @brief Тип обробника, який може бути викликано сервером у разі отримання нового запиту на авторизацію від клієнта.
     *
     */
    using ClientConfirmationHandler = std::function<void(const String name, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано сервером у разі втрати з'єднання з одним із клієнтів.
     *
     */
    using ClientDisconnectHandler = std::function<void(const String name, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано сервером у разі отримання пакета даних від одного із клієнтів.
     *
     */
    using ClientDataHandler = std::function<void(const ClientSession& client, const UdpPacket& packet, void* arg)>;

    GameServer();
    ~GameServer();

    /**
     * @brief Запускає сервер з вказаними параметрами.
     * Самостійно вмикає WiFi модуль, якщо is_local == true.
     * Інакше очікує, що з'єднання з точкою доступу вже встановлено.
     *
     * @param game_ID Ідентифікатор сервера гри
     * @param server_name Ім'я сервера, яке буде встановлено як ім'я точки доступу
     * @param pwd Пароль точки доступу
     * @param is_local Встановлює прапор, який вказує, чи буде сервер запущено на власній точці доступу(true), або ж в мережі іншого маршрутизатора(false)
     * @param max_connection Максимальна кількість клієнтів
     * @param wifi_chan Канал WiFi
     * @return true - Якщо сервер успішно запущено. false - інакше
     */
    bool begin(const String& game_ID, const String& server_name, const String& pwd, bool is_local = true, uint8_t max_connection = 1, uint8_t wifi_chan = 6);

    /**
     * @brief Скидає всі обробники подій, зупиняє сервер, звільняє ресурси та вимикає WiFi модуль.
     *
     */
    void stop();

    /**
     * @brief Вмикає можливість авторизації клієнтів, якщо на сервері доступні вільні слоти.
     *
     */
    void open();

    /**
     * @brief Вимикає можливість авторизації нових клієнтів.
     *
     */
    void close();

    /**
     * @brief Змінює стан сервера(закритий/відкритий) на протилежний.
     *
     */
    void toggle();

    /**
     * @brief Повертає значення прапора, який вказує на поточний стан можливості авторизації клієнтів на сервері.
     *
     * @return true - Якщо авторизація відкрита. false - інакше
     */
    bool isOpen() const;

    /**
     * @brief Повертає значення, яке вказує чи заповнено усі слоти сервера.
     *
     * @return true - Якщо усі слоти заповнено клієнтами
     * @return false - Якщо лишаються вільні слоти
     */
    bool isFull() const;

    /**
     * @brief Видаляє клієнта з сервера за вказаним ім'ям.
     *
     * @param client_name Рядок, що містить ім'я клієнта
     */
    void removeClient(const String& client_name);

    /**
     * @brief Видаляє клієнта з сервера за вказаною віддаленою ip-адресою.
     *
     * @param remote_ip Віддалена ip-адреса клієнта
     */
    void removeClient(IPAddress remote_ip);

    /**
     * @brief Підтверджує або відхиляє приєднання клієнта в залежності від результату його схвалення.
     *
     * @param client_name Ім'я клієнта
     * @param is_accepted Результат схвалення
     */
    void resolveJoin(const String& client_name, bool is_accepted);

    /**
     * @brief Надсилає пакет усім підключеним клієнтам.
     *
     * @param packet Пакет, що буде надіслано усім клієнтам
     */
    void sendBroadcast(const UdpPacket& packet);

    /**
     * @brief Формує та надсилає пакет усім підключеним клієнтам.
     *
     * @param type Тип пакета
     * @param data Дані, що будуть додані до пакета
     * @param data_size Розмір даних
     */
    void sendBroadcast(UdpPacket::PacketType type, const void* data, size_t data_size);

    /**
     * @brief Надсилає пакет за вказаною сесією клієнта.
     *
     * @param client Вказівник на клієнтську сесію
     * @param packet Пакет, що буде надіслано клієнту
     */
    void sendPacket(const ClientSession& client, const UdpPacket& packet);

    /**
     * @brief Формує та надсилає пакет за вказаною сесією клієнта.
     *
     * @param client Вказівник на клієнтську сесію
     * @param type Тип пакета
     * @param data Дані, що будуть додані до пакета
     * @param data_size Розмір даних
     */
    void sendPacket(const ClientSession& client, UdpPacket::PacketType type, const void* data, size_t data_size);

    /**
     * @brief Встановлює обробник, який буде викликано коли з'явиться новий запит на авторизацію клієнта.
     *
     * @param handler Обробник, що буде викликано у разі настання події
     * @param arg Аргумент, який буде передано обробнику
     */
    void onConfirmation(ClientConfirmationHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після втрати з'єднання з будь-яким із авторизованих клієнтів.
     *
     * @param handler Обробник, що буде викликано у разі настання події
     * @param arg Аргумент, який буде передано обробнику
     */
    void onDisconnect(ClientDisconnectHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета даних від будь-якого із авторизованих клієнтів.
     *
     * @param handler Обробник, що буде викликано у разі настання події
     * @param arg Аргумент, який буде передано обробнику
     */
    void onData(ClientDataHandler handler, void* arg);

    /**
     * @brief Повертає вказівник на список клієнтів.
     * Під час взаємодії зі списком, асинхронність не забезпечується.
     *
     * @return const std::unordered_map<uint32_t, ClientSession>*
     */
    const std::unordered_map<uint32_t, ClientSession>* getClients() const;

    /**
     * @brief Повертає локальну ip-адресу сервера.
     *
     * @return String
     */
    String getServerIP() const;

    /**
     * @brief Повертає ім'я сервера.
     *
     * @return String
     */
    String getName() const;

  protected:
    ClientSession* findClient(IPAddress remote_ip);
    ClientSession* findClient(const String& name);

    const ClientSession* findClient(IPAddress remote_ip) const;
    const ClientSession* findClient(const String& name) const;
    //
    void handlePacket(const UdpPacket& packet);
    static void packetHandlerTask(void* arg);
    //
    static void onPacket(void* arg, AsyncUDPPacket& packet);
    //
    void handleHandshake(const UdpPacket& packet);
    void handleName(ClientSession& client, const UdpPacket& packet);
    //
    void sendNameRespMsg(const ClientSession& client, bool result);
    void sendNameIncorrectMsg(const ClientSession& client);
    void sendBusyMsg(const ClientSession& client);
    //
    void invokeDataHandler(const ClientSession& client, const UdpPacket& packet);
    void invokeDisconnectHandler(const ClientSession& client);
    void invokeConfirmationHandler(const ClientSession& client);
    //
    void pingClients();
    static void pingClientsTask(void* arg);
    //
  protected:
    AsyncUDP _server;

    std::unordered_map<uint32_t, ClientSession> _clients;

    ClientConfirmationHandler _confirmation_handler{nullptr};
    ClientDisconnectHandler _disconnect_handler{nullptr};
    ClientDataHandler _data_handler{nullptr};

    String _server_name;
    String _game_id;
    String _server_ip;

    TaskHandle_t _ping_task_handler{nullptr};
    TaskHandle_t _packet_task_handler{nullptr};

    SemaphoreHandle_t _client_mutex{nullptr};
    SemaphoreHandle_t _udp_mutex{nullptr};

    QueueHandle_t _packet_queue{nullptr};

    void* _confirmation_arg{nullptr};
    void* _disconnect_arg{nullptr};
    void* _data_arg{nullptr};

    uint8_t _max_connection{1};
    uint8_t _confirmed_clients_num{0};

    bool _is_freed{true};
    bool _is_open{false};
    bool _is_busy{false};
  };
}  // namespace pixeler
