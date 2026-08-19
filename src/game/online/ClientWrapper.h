#pragma once
#pragma GCC optimize("O3")
#include <IPAddress.h>

#include "../../defines.h"
#include "UdpPacket.h"

namespace pixeler
{
  class ClientWrapper
  {
  public:
    /**
     * @brief Створює новий об'єкт.
     *
     * @param remote_ip Віддалена ip-адреса клієнта.
     * @param port Порт клієнта.
     */
    ClientWrapper(IPAddress remote_ip, uint16_t port);

    /**
     * @brief Повертає віддалену ip-адресу клієнта.
     *
     * @return IPAddress
     */
    IPAddress getIP() const;

    /**
     * @brief Повертає порт клієнта.
     *
     * @return uint16_t
     */
    uint16_t getPort() const;

    /**
     * @brief Встановлює прапор, який вказує, що клієнта було авторизовано на сервері.
     *
     */
    void confirm();

    /**
     * @brief Повертає стан прапору, який вказує чи було клієнта авторизовано на сервері.
     *
     * @return true - Якщо клієнт авторизований на сервері.
     * @return false - Інакше.
     */
    bool isConfirmed() const;

    /**
     * @brief Продовжує час з'єднання з цим клієнтом.
     *
     */
    void prolong();

    /**
     * @brief Повертає поточний стан з'єднаня з клієнтом.
     *
     * @return true - Якщо клієнт відповів на ping-пакет на протязі останніх 3 сек.
     * @return false - Інакше.
     */
    bool isConnected() const;

    /**
     * @brief Встановлює ім'я клієнта.
     *
     * @param name
     */
    void setName(const char* name);

    /**
     * @brief Повертає ім'я клієнта.
     *
     * @return const char*
     */
    const char* getName() const;

    /**
     * @brief Порівнює ім'я клієнта з переданим рядком.
     *
     * @param name Рядок, з яким буде порівняно ім'я клієнта.
     * @return true - Якщо рядок побайтово ідентичний з ім'ям клієнта.
     * @return false - Інакше.
     */
    bool hasName(const char* name) const;

    /**
     * @brief Порівнює два об'єкта ClientWrapper на основі віддаленої ip-адреси клієнтів.
     *
     * @param client_wrap Вказівник на інший об'єкт ClientWrapper.
     * @return true - Якщо віддалені ip-адреси співпадають.
     * @return false - Інакше.
     */
    bool is(const ClientWrapper* client_wrap) const;

  protected:
    ClientWrapper() {}

  protected:
    IPAddress _remote_ip;
    String _name;
    unsigned long _last_act_time{0};
    uint16_t _port{0};
    bool _is_confirmed{false};
  };
}  // namespace pixeler
