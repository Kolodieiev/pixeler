/**
 * @file WiFiManager.h
 * @brief Абстракція над бібліотекою WiFi
 * @details Покращує асинхронну взаємодію з модулем WiFi.
 * Зменшує дублювання часто використовуваного коду.
 */

#pragma once

#include <WiFi.h>

#include <vector>

#include "defines.h"

namespace pixeler
{
  static constexpr char STR_WIFI_SUBDIR[] = "wifi";
  static constexpr char STR_ROUTER_NOT_CONNECTED[] = "Не підключено до маршрутизатора";
  static constexpr uint8_t MAX_WIFI_CONNECTION = 10u;

  class WiFiManager
  {
  public:
    using ConnectCompleteHandler = std::function<void(void* arg, const String& ssid, wl_status_t conn_status)>;
    using ScanCompleteHandler = std::function<void(void* arg)>;

    enum WiFiPowerLevel : uint8_t
    {
      WIFI_POWER_MIN = 0,
      WIFI_POWER_MEDIUM,
      WIFI_POWER_MAX,
    };

    /**
     * @brief Запускає асинхронну спробу підключення до точки доступу із заданими параметрами.
     *
     * @param ssid Ім'я точки доступу
     * @param pwd Пароль точки доступу (порожній рядок для відкритої мережі)
     * @param wifi_chan Канал WiFi (0 - автовибір, 1..13 - фіксований канал)
     * @param autoreconnect Прапор, що вказує, чи потрібно виконувати автоматичну повторну спробу підключення
     * до точки доступу у разі невдачі або втрати з'єднання
     * @return true - Якщо запуск спроби підключення виконано успішно. false - Якщо не вдалося розпочати підключення
     */
    bool tryConnectTo(const String& ssid, const String& pwd, uint8_t wifi_chan = 0u, bool autoreconnect = false);

    /**
     * @brief Запускає асинхронну спробу підключення до відомої точки доступу із заданими параметрами.
     *
     * @param ssid Ім'я точки доступу
     * @param wifi_chan Канал WiFi (0 - автовибір, 1..13 - фіксований канал)
     * @param autoreconnect Прапор, що вказує, чи потрібно виконувати автоматичну повторну спробу підключення
     * до точки доступу у разі невдачі або втрати з'єднання
     * @return true - Якщо запуск спроби підключення виконано успішно. false - Якщо не вдалося розпочати підключення,
     * або точка доступу не є відомою
     */
    bool tryConnectToKnown(const String& ssid, uint8_t wifi_chan = 0u, bool autoreconnect = false);

    /**
     * @brief Зберігає ім'я точки доступу та пароль до неї без шифрування на карту пам'яті.
     *
     * @param ssid Ім'я точки доступу
     * @param pwd Пароль точки доступу
     * @return true - Якщо збереження було виконано успішно. false - Інакше
     */
    bool saveSSID(const String& ssid, const String& pwd) const;

    /**
     * @brief Видаляє збережену раніше точку доступу з карти пам'яті.
     *
     * @param ssid Ім'я точки доступу
     * @return true - Якщо видалення було виконано успішно. false - Інакше
     */
    bool forgetSSID(const String& ssid) const;

    /**
     * @brief Повертає значення, яке вказує, чи збережено пароль до точки на карті пам'яті.
     *
     * @param ssid Ім'я точки доступу
     * @return true - Якщо пароль до точки доступу збережено на карті пам'яті. false - Інакше
     */
    bool hasKnownSSID(const String& ssid) const;

    /**
     * @brief Повертає пароль від точки доступу, яку було збережено раніше на карту пам'яті.
     *
     * @param ssid Ім'я точки доступу
     * @return String - пароль від точки доступу.
     * Порожній рядок, якщо не вдалося отримати збережене значення
     */
    String getSSIDKey(const String& ssid) const;

    /**
     * @brief Створює точку доступу WiFi, до якої можуть приєднуватися WiFi-клієнти.
     *
     * @param ssid Ім'я точки доступу
     * @param pwd Пароль точки доступу (порожній рядок або від 8 до 63 символів)
     * @param max_connection Максимальна кількість підключень (від 1 до 10).
     * Рекомендується встановлювати менше 5 одночасних підключень
     * @param wifi_chan Канал WiFi (від 1 до 13)
     * @param is_hidden Якщо true - точку доступу буде створено прихованою
     * @return true - Якщо точку доступу створено успішно. false - Інакше
     */
    bool createAP(const String& ssid, const String& pwd, uint8_t max_connection = 1u, uint8_t wifi_chan = 1u, bool is_hidden = false);

    /**
     * @brief Встановлює обробник події, який буде викликано після завершення спроби підключення до точки доступу.
     *
     * @param handler Асинхронний обробник події завершення спроби підключення до точки доступу
     * @param arg Аргумент, що буде передано обробнику
     */
    void onConnectComplete(ConnectCompleteHandler handler, void* arg);

    /**
     * @brief Встановлює обробник події, який буде викликано після завершення сканування точок WiFi.
     *
     * @param handler Асинхронний обробник події завершення сканування
     * @param arg Аргумент, що буде передано обробнику
     */
    void onScanComplete(ScanCompleteHandler handler, void* arg);

    /**
     * @brief Налаштовує WiFi модуль та запускає сканування точок доступу.
     *
     * @return true - Якщо сканування було запущено. false - Інакше
     */
    bool startScan();

    /**
     * @brief Повертає вектор з іменами всіх виявлених під час останнього сканування точок доступу.
     * Після повернення вектора результат попереднього сканування очищується автоматично.
     *
     * @return std::vector<String>
     */
    std::vector<String> getScanResult();

    /**
     * @brief Тимчасово встановлює потужність передачі модуля WiFi.
     *
     * @param power_lvl Значення перечислення потужності модуля.
     * WIFI_POWER_MIN == 5 dBm.
     * WIFI_POWER_MEDIUM == 15 dBm.
     * WIFI_POWER_MAX == 19.5 dBm
     * @return true - Якщо потужність модуля встановлено успішно. false - Інакше
     */
    bool setPower(WiFiPowerLevel power_lvl);

    /**
     * @brief Встановлює потужність передачі модуля WiFi (якщо модуль увімкнено)
     * та зберігає налаштування потужності на карту пам'яті.
     * Налаштування будуть зчитані та застосовані автоматично під час наступного увімкнення модуля WiFi.
     *
     * @param power_lvl Рівень потужності WiFi
     * @return true - Якщо налаштування успішно збережено на карту пам'яті. false - Інакше
     */
    bool savePower(WiFiPowerLevel power_lvl);

    /**
     * @brief Повертає поточне значення потужності передавання модуля WiFi.
     * Значення можна вважати актуальним лише за умови, що модуль WiFi було увімкнено раніше.
     *
     * @return WiFiPowerLevel
     */
    WiFiPowerLevel getPower() const;

    /**
     * @brief Повертає логічне значення, яке вказує на те, чи встановлено з'єднання з WiFi.
     *
     * @return true - Якщо з'єднання встановлено. false - Інакше
     */
    bool isConnected() const;

    /**
     * @brief Повертає логічне значення, яке вказує на те, чи активовано точку доступу на пристрої.
     *
     * @return true - Якщо точка доступу активна. false - Інакше
     */
    bool isApEnabled() const;

    /**
     * @brief Повертає назву WiFi-точки доступу, з якою встановлено з'єднання.
     *
     * @return String - SSID точки доступу, якщо з'єднання існує, або порожній рядок, якщо з'єднання відсутнє
     */
    String getSSID() const;

    /**
     * @brief Відключається від точки доступу, якщо було встановлено з'єднання.
     */
    void disconnect();

    /**
     * @brief Повертає стан модуля WiFi.
     *
     * @return true - Якщо модуль WiFi увімкнено. false - Інакше
     */
    bool isEnabled() const;

    /**
     * @brief Вмикає модуль WiFi у режимі STA.
     *
     * @return true - Якщо модуль WiFi було успішно увімкнено. false - Інакше
     */
    bool enable();

    /**
     * @brief Зупиняє поточні операції, відключається від точки доступу та вимикає модуль WiFi.
     */
    void disable();

    /**
     * @brief Перемикає поточний стан модуля WiFi на протилежний.
     *
     * @return true - Якщо операцію виконано успішно. false - Інакше
     */
    bool toggle();

    /**
     * @brief Повертає IP-адресу пристрою.
     *
     * @return String - IP-адреса пристрою, якщо пристрій знаходиться в мережі. Порожній рядок - інакше
     */
    String getIP() const;

    /**
     * @brief Повертає стан прапора, який вказує на те, чи зайнятий зараз модуль WiFi асинхронною роботою.
     *
     * @return true - Якщо модуль зайнятий. false - Інакше
     */
    bool isBusy() const;

    WiFiManager();
    ~WiFiManager();
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;
    WiFiManager(WiFiManager&&) = delete;
    WiFiManager& operator=(WiFiManager&&) = delete;

  private:
    enum OperationState : uint8_t
    {
      OP_IDLE = 0,
      OP_PREPARING,
      OP_SCANNING,
      OP_CONNECTING,
    };

    bool ensureEventsRegistered();
    void recoverIfTimedOut();
    void resetOpState(OperationState expected_state);

    WiFiPowerLevel readPowerSettings() const;
    bool savePowerSettings(WiFiPowerLevel power_lvl) const;

    void handleConnDone(wl_status_t conn_status);
    void handleScanDone();

    static void onEvent(WiFiEvent_t event);

  private:
    ScanCompleteHandler _scan_complete_handler{nullptr};
    ConnectCompleteHandler _connect_complete_handler{nullptr};
    String _last_ssid;

    SemaphoreHandle_t _mutex{nullptr};
    void* _scan_complete_handler_arg{nullptr};
    void* _connect_complete_handler_arg{nullptr};
    uint32_t _op_start_ms{0u};
    uint32_t _op_timeout_ms{0u};

    OperationState _op_state{OP_IDLE};
    bool _events_registered{false};
  };

  /**
   * @brief Глобальний об'єкт для роботи з модулем WiFi.
   */
  extern WiFiManager _wifi;
}  // namespace pixeler
