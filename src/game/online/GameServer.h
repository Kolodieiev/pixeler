#pragma once
#pragma GCC optimize("O3")
#include <AsyncUDP.h>

#include <unordered_map>

#include "AckTracker.h"
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
    using AcceptHandler = std::function<void(const String name, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано сервером у разі втрати з'єднання з одним із клієнтів.
     *
     */
    using DisconnectHandler = std::function<void(const IPAddress& client_ip, void* arg)>;

    /**
     * @brief Тип обробника, який може бути викликано сервером у разі отримання пакета даних від одного із клієнтів.
     *
     */
    using DataHandler = std::function<void(const ClientSession& session, const UdpPacket& packet, void* arg)>;

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
     * @param max_clients Максимальна кількість клієнтів від 1 до 10 включно
     * @param is_local Встановлює прапор, який вказує, чи буде сервер запущено на власній точці доступу(true), або ж в мережі іншого маршрутизатора(false)
     * @param wifi_chan Канал WiFi
     * @return true - Якщо сервер успішно запущено. false - інакше
     */
    bool begin(const String& game_ID, const String& server_name, const String& pwd, uint8_t max_clients = 1, bool is_local = true, uint8_t wifi_chan = 6);

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
     * @brief Повертає віддалену IP-адресу клієнта по його імені.
     *
     * @param client_name Ім'я клієнта
     * @return IPAddress - Віддалена IP-адреса клієнта.
     * Або 0 якщо клієнт з таким іменем не авторизований
     */
    IPAddress getClientIP(const String& client_name);

    /**
     * @brief Видаляє сеанс клієнта з сервера за вказаним ім'ям.
     * Якщо клієнт був підтверджений, після його видалення буде викликано onDisconnect.
     *
     * @param client_name Рядок, що містить ім'я клієнта
     */
    void removeSession(const String& client_name);

    /**
     * @brief Видаляє сеанс клієнта з сервера за вказаною віддаленою ip-адресою.
     * Якщо клієнт був підтверджений, після його видалення буде викликано onDisconnect.
     *
     * @param remote_ip Віддалена ip-адреса клієнта
     */
    void removeSession(const IPAddress& remote_ip);

    /**
     * @brief Підтверджує або відхиляє приєднання клієнта в залежності від результату його схвалення.
     *
     * @param client_name Ім'я клієнта
     * @param is_accepted Результат схвалення
     */
    void resolveJoin(const String& client_name, bool is_accepted);

    /**
     * @brief Формує та надсилає всім авторизованим клієнтам повідомлення про початок гри.
     *
     */
    void broadcastStartGame();

    /**
     * @brief Формує та надсилає всім авторизованим клієнтам повідомлення про завершення гри.
     *
     */
    void broadcastStopGame();

    /**
     * @brief Формує та надсилає пакет усім підключеним клієнтам.
     *
     * @param type Тип пакета
     * @param subtype Підтип основго типу пакета
     * @param data_size Розмір даних
     * @param data Дані, що будуть додані до пакета
     */
    bool broadcast(UdpPacket::PacketType type, uint8_t subtype = 0, size_t data_size = 0, const void* data = nullptr);

    /**
     * @brief Формує та надсилає пакет за вказаною сесією клієнта.
     *
     * @param session Вказівник на клієнтську сесію
     * @param type Тип пакета
     * @param subtype Підтип основго типу пакета
     * @param data_size Розмір даних
     * @param data Дані, що будуть додані до пакета
     */
    bool send(const ClientSession& session, UdpPacket::PacketType type, uint8_t subtype = 0, size_t data_size = 0, const void* data = nullptr);

    /**
     * @brief Формує та надсилає пакет за вказаною віддаленою IP-адресою клієнта.
     *
     * @param remote_ip Віддалена IP-адреса клієнта
     * @param type Тип пакета
     * @param subtype Підтип основго типу пакета
     * @param data_size Розмір даних
     * @param data Дані, що будуть додані до пакета
     */
    bool send(const IPAddress& remote_ip, UdpPacket::PacketType type, uint8_t subtype = 0, size_t data_size = 0, const void* data = nullptr);

    /**
     * @brief Повторно надсилає останній пакет клієнтам, які не надіслали ACK-пакет.
     * Видаляє всіх клієнтів, які не надіслали ACK, за відповідну кількість запитів.
     * Якщо клієнт був підтверджений, після його видалення буде викликано onDisconnect.
     *
     */
    void requestAcks();

    /**
     * @brief Перевіряє, чи всі клієнти надіслали ACK на попередній пакет.
     *
     * @return true - Якщо всі клієнти відповіли на попередній пакет.
     * false - Інакше
     */
    bool isAllAcked() const;

    /**
     * @brief Встановлює обробник, який буде викликано коли з'явиться новий запит на авторизацію клієнта.
     *
     * @param handler Обробник, що буде викликано у разі настання події
     * @param arg Аргумент, який буде передано обробнику
     */
    void onAccept(AcceptHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після втрати з'єднання
     * з будь-яким із авторизованих клієнтів.
     *
     * @param handler Обробник, що буде викликано у разі настання події
     * @param arg Аргумент, який буде передано обробнику
     */
    void onDisconnect(DisconnectHandler handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано після отримання пакета ігрових даних
     * від будь-якого із авторизованих клієнтів.
     *
     * @param handler Обробник, що буде викликано у разі отримання ігрових даних
     * @param arg Аргумент, який буде передано обробнику
     */
    void onGameData(DataHandler handler, void* arg);

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
    ClientSession* findSession(const IPAddress& remote_ip);
    ClientSession* findSession(const String& name);

    const ClientSession* findSession(const IPAddress& remote_ip) const;
    const ClientSession* findSession(const String& name) const;

    ClientSession* findSessionUnsafe(const IPAddress& remote_ip);

    void removeSessionUnsafe(const IPAddress& remote_ip);
    //
    void handlePacket(const UdpPacket& packet);
    static void packetHandlerTask(void* arg);
    //
    static void onPacket(void* arg, AsyncUDPPacket& packet);
    //
    void handleHandshake(const UdpPacket& packet);
    void handleLogin(ClientSession& session, const UdpPacket& packet);
    bool openSession(const UdpPacket& packet);
    //
    void sendNameRespMsg(const ClientSession& session, bool result);
    void sendIncorrectName(const ClientSession& session);
    void sendBusy(const UdpPacket& packet);
    //
    void invokeDataHandler(const ClientSession& session, const UdpPacket& packet);
    void handleAck(const UdpPacket& packet);
    void invokeDisconnectHandler(const IPAddress& remote_ip);
    void invokeAcceptHandler(const ClientSession& session, const String& client_name);
    //
    void pingClients();
    static void pingClientsTask(void* arg);
    //
    bool broadcast(const UdpPacket& packet);
    bool send(const IPAddress& remote_ip, const UdpPacket& packet);
    bool send(const ClientSession& session, const UdpPacket& packet);
    void sendUnacked(const UdpPacket& packet, const IPAddress& remote_ip);
    void sendUnacked(const UdpPacket& packet, const IPAddress& remote_ip, uint16_t port);
    void sendUnackedUnsafe(const UdpPacket& packet, const IPAddress& remote_ip);
    void sendUnackedUnsafe(const UdpPacket& packet, const IPAddress& remote_ip, uint16_t port);

  protected:
    AsyncUDP _server;
    UdpPacket _last_packet;
    std::unordered_map<uint32_t, ClientSession> _sessions;
    AckTracker _ack_tracker;

    AcceptHandler _accept_handler{nullptr};
    DisconnectHandler _disconnect_handler{nullptr};
    DataHandler _data_handler{nullptr};

    String _server_name;
    String _game_id;
    String _server_ip;

    TaskHandle_t _ping_task_handler{nullptr};
    TaskHandle_t _packet_task_handler{nullptr};

    mutable SemaphoreHandle_t _sessions_mutex{nullptr};
    mutable SemaphoreHandle_t _udp_mutex{nullptr};

    QueueHandle_t _packet_queue{nullptr};

    void* _accept_arg{nullptr};
    void* _disconnect_arg{nullptr};
    void* _data_arg{nullptr};

    uint8_t _last_packet_id{0};
    uint8_t _resend_counter{0};

    uint8_t _max_clients{1};
    uint8_t _accepted_sessions_num{0};

    bool _is_open{false};
    bool _is_freed{true};
    bool _is_busy{false};
    bool _wifi_was_enabled{false};
  };
}  // namespace pixeler
