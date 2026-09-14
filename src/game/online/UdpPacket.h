#pragma once
#pragma GCC optimize("O3")
#include <AsyncUDP.h>

#include "../DataStream.h"
#include "defines.h"

namespace pixeler
{
  static const uint8_t PACKET_EXTRA_SIZE = 4;    // Основний тип пакета 1 байт + його підтип 1 байт + id 1 байт.
  static const uint16_t MAX_PACKET_SIZE = 1000;  // Максимальний розмір пакета.

  class UdpPacket : public DataStream
  {
  public:
    UdpPacket(const UdpPacket&);
    UdpPacket& operator=(const UdpPacket&);

    UdpPacket(UdpPacket&&);
    UdpPacket& operator=(UdpPacket&&);

    /**
     * @brief Перечислення, що містить базовий тип пакета.
     *
     */
    enum PacketType : uint8_t
    {
      TYPE_PING,          // Пакет для перевірки стану з'єднання.
      TYPE_GAME_DATA,     // Пакет для обміну ігровими даними.
      TYPE_SERVICE_DATA,  // Пакет для обміну сервісними даними між сервером та клієнтом.
      TYPE_CONNECT,       // Пакет для обміну даними про підключення.
    };

    /**
     * @brief Перечислення, що містить підтипи пакета підключення.
     *
     */
    enum PackConnectSubtype : uint8_t
    {
      SUBTYPE_HANDSHAKE,         // Пакет для розпізнавання сервера.
      SUBTYPE_INCORRECT_SERVER,  // Пакет, що вказує на некоректність сервера.
      SUBTYPE_ACCESS_GRANTED,    // Пакет, що вказує у наданні доступу.
      SUBTYPE_ACCESS_DENIED,     // Пакет, що вказує у відмові доступу.
      SUBTYPE_LOGIN,             // Пакет, що містить ім'я клієнта.
      SUBTYPE_INCORRECT_NAME,    // Пакет, що вказує на некоректність імені клієнта.
      SUBTYPE_BUSY,              // Пакет, що вказує на зайнятість системи авторизації.
    };

    /**
     * @brief Перечислення, що містить підтипи пакета клієнтських даних.
     *
     */
    enum PackServiceSubtype : uint8_t
    {
      SUBTYPE_START_GAME,  // Пакет, який вказує на необхідність запуску гри.
      SUBTYPE_STOP_GAME,   // Пакет, який вказує на необхідність завершення гри.
      SUBTYPE_ACK,         // Пакет, що містить підтвердження про отримання пакету.
    };

    /**
     * @brief Створює новий об'єкт на основі даних об'єкта AsyncUDPPacket.
     *
     * @param packet
     */
    explicit UdpPacket(AsyncUDPPacket& packet);

    /**
     * @brief Створює новий об'єкт без місця під дані.
     *
     */
    UdpPacket();

    /**
     * @brief Створює новий об'єкт та виділяє місце під дані.
     *
     * @param data_len Розмір буфера даних
     */
    explicit UdpPacket(size_t data_len);

    /**
     * @brief Встановлює тип пакета.
     *
     * @param type Тип пакета
     */
    void setType(PacketType type);

    /**
     * @brief Повертає тип пакета.
     *
     * @return PacketType
     */
    PacketType getType() const;

    /**
     * @brief Встановлює підтип основго типу пакета.
     *
     * @param subtype
     */
    void setSubtype(uint8_t subtype);

    /**
     * @brief Повертає підтип основго типу пакета.
     *
     * @return uint8_t
     */
    uint8_t getSubtype() const;

    /**
     * @brief Встановлює ідентифікатор пакета.
     *
     * @param id
     */
    void setID(uint8_t id);

    /**
     * @brief Повертає значення, яке вказує, чи було задано цьому пакету ID.
     *
     * @return true - Якщо ідентифікатор було присвоєно пакету.
     * @return false - Інакше
     */
    bool hasID() const;

    /**
     * @brief Повертає ідентифікатор пакета.
     *
     * @return uint8_t
     */
    uint8_t getID() const;

    /**
     * @brief Повертає вказівник із заданим зміщенням на дані пакета.
     * Дані за вказівником не повинні бути видалені або змінені.
     * В іншому випадку, поведінка програми невизначена.
     *
     * @param data_pos Зміщення відносно початку даних пакета
     * @return const char* - Вказівник на дані з урахуванням зміщення.
     * Якщо зміщення перевищує кількість даних - вказівник на 0-вий байт даних
     */
    const uint8_t* getData(uint16_t data_pos = 0) const;

    /**
     * @brief Повертає дані пакета у вигляді текстового рядка.
     *
     * @return String
     */
    String dataToString() const;

    /**
     * @brief Повертає розмір даних пакета.
     *
     * @return size_t
     */
    size_t getDataLen() const;

    /**
     * @brief Виводить дані пакета до UART.
     * Використовується для відлагодження.
     *
     * @param char_like Прапор, який вказує на формат виводу даних.
     * Якщо true кожен байт виводиться - як символ.
     * Інакше як hex-число
     */
    void printToLog(bool char_like = true) const;

    /**
     * @brief Повертає віддалену ip-адресу, з якої було отримано цей пакет.
     * Значення буде не 0 тільки у випадку, якщо пакет було сконструйовано на основі даних об'єкта AsyncUDPPacket.
     *
     * @return IPAddress
     */
    IPAddress getIP() const;

    /**
     * @brief Повертає порт, з якого було отримано цей пакет.
     * Значення буде не 0 тільки у випадку, якщо пакет було сконструйовано на основі даних об'єкта AsyncUDPPacket.
     *
     * @return uint16_t
     */
    uint16_t getPort() const;

    /**
     * @brief Порівнює побайтово дані з заданої позиції.
     *
     * @param data Буфер, з яким будуть порівнюватися дані пакета
     * @param data_len Кількість байтів, які потрібно порівняти. Якщо 0 - порівняти до кінця даних пакета
     * @param start_pos Зміщення в буфері даних пакета
     * @return true - якщо байти даних збігаються. false - інакше
     */
    bool isDataEquals(const void* data, size_t data_len = 0, size_t start_pos = 0) const;

    /**
     * @brief Копіює вказану кількість байтів даних у зовнішній буфер.
     *
     * @param out Вказівник на зовнішній буфер, куди буде скопійовано байти даних
     * @param start_pos Початкова позиція звідки необхідно почати копіювати дані
     * @param len Кількість байтів, які необхідно скопіювати
     * @return size_t - Кількість скопійованих байтів
     */
    size_t extractBytes(void* out, size_t start_pos = 0, size_t len = 1) const;

    /**
     * @brief Скидає позицію каретки відносно даних.
     * Дані в буфері залишаються без змін.
     *
     */
    void resetDataIndex();

    /**
     * @brief Повертає поточну позицію каретки в буфері відносно секції даних.
     *
     * @return size_t
     */
    size_t getDataIndex() const;

  private:
    using DataStream::resize;

    IPAddress _remote_ip;
    size_t _data_length{0};
    uint16_t _port{0};
  };
}  // namespace pixeler
