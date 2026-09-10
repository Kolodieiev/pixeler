#pragma once
#pragma GCC optimize("O3")
#include <stdint.h>

#include <vector>

#include "IPAddress.h"

namespace pixeler
{
  class AckTracker
  {
  public:
    AckTracker();
    ~AckTracker();

    AckTracker(const AckTracker&) = delete;
    AckTracker& operator=(const AckTracker&) = delete;

    AckTracker(AckTracker&&) = delete;
    AckTracker& operator=(AckTracker&&) = delete;

    /**
     * @brief Ініціалізує трекер заданою кількістю максимальних підключень.
     *
     */
    void init(uint8_t max_clients = 10);

    /**
     * @brief Додає нового клієнта до трекера отримання пакетів.
     *
     * @param ip Віддалена IP-адреса клієнта
     * @return true - Якщо клієнт успішно додано зараз або раніше.
     * false - Інакше.
     */
    bool addClient(const IPAddress& ip);

    /**
     * @brief Повертає список клієнтів, які ще не повернули підтвердження отримання пакету.
     *
     * @return std::vector<IPAddress>
     */
    std::vector<IPAddress> getUnackedClients() const;

    /**
     * @brief Видаляє клієнта з трекера за його IP-адресою.
     *
     * @param ip Віддалена IP-адреса клієнта
     */
    void removeClient(const IPAddress& ip);

    /**
     * @brief Скидає всі ACK прапори клієнтів.
     *
     */
    void resetAllAcks();

    /**
     * @brief Скидає ACK-прапор для одного клієнта за його IP-адресою
     *
     * @param ip Віддалена IP-адреса клієнта
     */
    void resetAck(const IPAddress& ip);

    /**
     * @brief Позначає клієнта таким, що прислав ACK.
     *
     * @param ip Віддалена IP-адреса клієнта
     */
    void setAck(const IPAddress& ip);

    /**
     * @brief Перевіряє, чи всі клієнти надіслали ACK на попередній пакет.
     *
     * @return true - Якщо всі клієнти відповіли на попередній пакет.
     * false - Інакше
     */
    bool isAllAcked() const;

    /**
     * @brief Повертає поточну кількість клієнтів, які відслідковуються.
     *
     * @return uint8_t
     */
    uint8_t getClientsCount() const;

  private:
    int findClientIndex(const IPAddress& ip) const;

  private:
    IPAddress* _clients{nullptr};

    uint32_t _active_mask{0};
    uint32_t _ack_mask{0};

    uint8_t _max_client_num{1};
    uint8_t _clients_num{0};
  };
}  // namespace pixeler
