#pragma once
#pragma GCC optimize("O3")
#include <IPAddress.h>

#include "UdpPacket.h"
#include "defines.h"

namespace pixeler
{
  class GameServer;

  class ClientSession
  {
  public:
    /**
     * @brief Створює новий об'єкт сесії клієнта.
     *
     * @param remote_IP Віддалена IP-адреса клієнта
     * @param port Порт клієнта
     */
    ClientSession(IPAddress remote_IP, uint16_t port);

    /**
     * @brief Повертає віддалену IP-адресу клієнта.
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
     * @brief Повертає поточний стан підключення.
     *
     * @return true - якщо клієнт відповів на ping-пакет протягом останніх 3 сек. false - інакше
     */
    bool isConnected() const;

    /**
     * @brief Повертає ім'я клієнта.
     *
     * @return String
     */
    String getName() const;

    /**
     * @brief Порівнює ім'я клієнта з переданим рядком.
     *
     * @param name Рядок, з яким буде порівняно ім'я клієнта
     * @return true - якщо рядок побайтово ідентичний з ім'ям клієнта. false - інакше
     */
    bool hasName(const String& name) const;

    /**
     * @brief Порівнює два об'єкта ClientSession на основі віддаленої IP-адреси клієнтів.
     *
     * @param session Адреса ClientSession
     * @return true - якщо віддалені IP-адреси збігаються. false - інакше
     */
    bool is(const ClientSession& session) const;

  private:
    friend class GameServer;

    /**
     * @brief Встановлює прапор, який вказує, що клієнта з цією сесією було авторизовано на сервері.
     *
     */
    void confirm();

    /**
     * @brief Повертає стан прапора, який вказує чи було авторизовано клієнта з цією сесією на сервері.
     *
     * @return true - якщо клієнт авторизований на сервері. false - інакше
     */
    bool isConfirmed() const;

    /**
     * @brief Продовжує підключення сесії.
     *
     */
    void prolong();

    /**
     * @brief Встановлює ім'я клієнта з цією сесією.
     *
     * @param name
     */
    void setName(const String& name);

  protected:
    ClientSession() {}

  protected:
    IPAddress _remote_IP;
    String _name;
    unsigned long _last_act_time{0};
    uint16_t _port{0};
    bool _is_confirmed{false};
  };
}  // namespace pixeler
