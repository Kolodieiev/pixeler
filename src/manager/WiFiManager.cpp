#include "WiFiManager.h"

#include <esp_wifi.h>

#include <algorithm>

#include "SettingsManager.h"

static constexpr char STR_PREF_WIFI_POWER[] = "wifipower";

static constexpr char STR_ERR_WIFI_BUSY[] = "WiFi-модуль зайнятий";
static constexpr char STR_ERR_EMPTY_SSID[] = "SSID не може бути порожній";
static constexpr char STR_ERR_UNKNOWN_SSID[] = "Невідомий SSID:";

static constexpr uint8_t MAX_WIFI_CHAN{13u};
static constexpr uint8_t MIN_AP_PWD_LEN{8u};
static constexpr uint8_t MAX_AP_PWD_LEN{63u};

static constexpr uint32_t SCAN_TIMEOUT_MS{15000u};
static constexpr uint32_t CONNECT_TIMEOUT_MS{15000u};

namespace pixeler
{
  WiFiManager::WiFiManager() : _mutex{xSemaphoreCreateMutex()}
  {
  }

  WiFiManager::~WiFiManager()
  {
    if (_mutex)
    {
      vSemaphoreDelete(_mutex);
      _mutex = nullptr;
    }
  }

  bool WiFiManager::ensureEventsRegistered()
  {
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (!_events_registered)
    {
      WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_SCAN_DONE);
      WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
      WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);
      _events_registered = true;
    }

    xSemaphoreGive(_mutex);
    return true;
  }

  void WiFiManager::recoverIfTimedOut()
  {
    OperationState timed_out_state{OP_IDLE};

    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    if ((_op_state == OP_SCANNING || _op_state == OP_CONNECTING) && (millis() - _op_start_ms) >= _op_timeout_ms)
    {
      timed_out_state = _op_state;
      _op_state = OP_IDLE;
    }

    xSemaphoreGive(_mutex);

    if (timed_out_state == OP_SCANNING)
    {
      log_e("Перевищено час очікування сканування WiFi");
      esp_wifi_scan_stop();
      WiFi.scanDelete();
    }
    else if (timed_out_state == OP_CONNECTING)
    {
      log_e("Перевищено час очікування підключення WiFi");
      WiFi.disconnect(false);
    }
  }

  void WiFiManager::resetOpState(const OperationState expected_state)
  {
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    if (_op_state == expected_state)
      _op_state = OP_IDLE;

    xSemaphoreGive(_mutex);
  }

  bool WiFiManager::tryConnectTo(const String& ssid, const String& pwd, uint8_t wifi_chan, const bool autoreconnect)
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    recoverIfTimedOut();

    if (!ensureEventsRegistered())
      return false;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (_op_state != OP_IDLE)
    {
      xSemaphoreGive(_mutex);
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    // OP_PREPARING блокує паралельні операції та ігнорує події розриву попереднього з'єднання
    _op_state = OP_PREPARING;
    xSemaphoreGive(_mutex);

    if (isApEnabled())
      WiFi.softAPdisconnect(false);

    if (WiFi.getMode() == WIFI_MODE_STA)
    {
      const bool was_connected = isConnected();
      WiFi.disconnect(false);
      if (was_connected)
        delay(100);
    }
    else
    {
      WiFi.persistent(false);
      if (!WiFi.mode(WIFI_MODE_STA))
      {
        resetOpState(OP_PREPARING);
        log_e("Помилка увімкнення режиму WiFi STA");
        return false;
      }

      setPower(readPowerSettings());
    }

    if (wifi_chan > MAX_WIFI_CHAN)
      wifi_chan = MAX_WIFI_CHAN;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (_op_state != OP_PREPARING)
    {
      xSemaphoreGive(_mutex);
      return false;
    }

    _last_ssid = ssid;
    _op_state = OP_CONNECTING;
    _op_start_ms = millis();
    _op_timeout_ms = CONNECT_TIMEOUT_MS;
    xSemaphoreGive(_mutex);

    WiFi.setAutoReconnect(autoreconnect);
    WiFi.persistent(false);
    const wl_status_t status = WiFi.begin(ssid.c_str(), pwd.isEmpty() ? nullptr : pwd.c_str(), wifi_chan);

    if (status == WL_CONNECT_FAILED)
    {
      log_e("Помилка приєднання до: %s", ssid.c_str());
      resetOpState(OP_CONNECTING);
      return false;
    }

    return true;
  }

  bool WiFiManager::tryConnectToKnown(const String& ssid, const uint8_t wifi_chan, const bool autoreconnect)
  {
    const String pwd = getSSIDKey(ssid);
    if (pwd.isEmpty())
      return false;

    return tryConnectTo(ssid, pwd, wifi_chan, autoreconnect);
  }

  bool WiFiManager::saveSSID(const String& ssid, const String& pwd) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    return SettingsManager::set(ssid, pwd, STR_WIFI_SUBDIR);
  }

  bool WiFiManager::forgetSSID(const String& ssid) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    return SettingsManager::remove(ssid, STR_WIFI_SUBDIR);
  }

  bool WiFiManager::hasKnownSSID(const String& ssid) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    const String pwd = SettingsManager::get(ssid, STR_WIFI_SUBDIR);
    return !pwd.isEmpty();
  }

  String WiFiManager::getSSIDKey(const String& ssid) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return emptyString;
    }

    String pwd = SettingsManager::get(ssid, STR_WIFI_SUBDIR);
    if (pwd.isEmpty())
      log_e("%s %s", STR_ERR_UNKNOWN_SSID, ssid.c_str());

    return pwd;
  }

  bool WiFiManager::createAP(const String& ssid, const String& pwd, const uint8_t max_connection, uint8_t wifi_chan, const bool is_hidden)
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    if (!pwd.isEmpty() && (pwd.length() < MIN_AP_PWD_LEN || pwd.length() > MAX_AP_PWD_LEN))
    {
      log_e("Пароль точки доступу має містити від %u до %u символів", MIN_AP_PWD_LEN, MAX_AP_PWD_LEN);
      return false;
    }

    recoverIfTimedOut();

    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (_op_state != OP_IDLE)
    {
      xSemaphoreGive(_mutex);
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    _op_state = OP_PREPARING;
    xSemaphoreGive(_mutex);

    if (isConnected())
      WiFi.disconnect(false);

    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_MODE_AP))
    {
      resetOpState(OP_PREPARING);
      log_e("Помилка переходу в режим точки доступу");
      return false;
    }

    wifi_chan = std::clamp<uint8_t>(wifi_chan, 1u, MAX_WIFI_CHAN);
    const uint8_t clamped_conn = std::clamp<uint8_t>(max_connection, 1u, MAX_WIFI_CONNECTION);
    const bool result = WiFi.softAP(ssid.c_str(), pwd.isEmpty() ? nullptr : pwd.c_str(), wifi_chan, is_hidden, clamped_conn);
    delay(100);

    if (result)
      setPower(readPowerSettings());
    else
      log_e("Помилка створення точки доступу");

    resetOpState(OP_PREPARING);
    return result;
  }

  void WiFiManager::onConnectComplete(ConnectCompleteHandler handler, void* arg)
  {
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    _connect_complete_handler = std::move(handler);
    _connect_complete_handler_arg = arg;
    xSemaphoreGive(_mutex);
  }

  void WiFiManager::onScanComplete(ScanCompleteHandler handler, void* arg)
  {
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    _scan_complete_handler = std::move(handler);
    _scan_complete_handler_arg = arg;
    xSemaphoreGive(_mutex);
  }

  bool WiFiManager::startScan()
  {
    recoverIfTimedOut();

    if (!ensureEventsRegistered())
      return false;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (_op_state != OP_IDLE)
    {
      xSemaphoreGive(_mutex);
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    _op_state = OP_PREPARING;
    xSemaphoreGive(_mutex);

    if (WiFi.getMode() != WIFI_MODE_STA)
    {
      if (isApEnabled())
        WiFi.softAPdisconnect(false);

      WiFi.persistent(false);
      if (!WiFi.mode(WIFI_MODE_STA))
      {
        resetOpState(OP_PREPARING);
        return false;
      }

      setPower(readPowerSettings());
    }
    else if (!isConnected())
    {
      // Зупиняємо можливу фонову спробу перепідключення, яка блокує esp_wifi_scan_start()
      WiFi.disconnect(false);
    }

    WiFi.scanDelete();

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    if (_op_state != OP_PREPARING)
    {
      xSemaphoreGive(_mutex);
      return false;
    }

    _op_state = OP_SCANNING;
    _op_start_ms = millis();
    _op_timeout_ms = SCAN_TIMEOUT_MS;
    xSemaphoreGive(_mutex);

    const int16_t result_code = WiFi.scanNetworks(true);
    if (result_code != WIFI_SCAN_RUNNING)
    {
      log_e("Помилка запуску сканера Wi-Fi: %d", result_code);
      resetOpState(OP_SCANNING);
      return false;
    }

    return true;
  }

  std::vector<String> WiFiManager::getScanResult()
  {
    std::vector<String> out_vector;

    recoverIfTimedOut();

    if (isBusy())
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return out_vector;
    }

    const int16_t scan_result = WiFi.scanComplete();

    if (scan_result == WIFI_SCAN_RUNNING)
    {
      log_e("Сканування ще не завершено");
      return out_vector;
    }
    else if (scan_result == WIFI_SCAN_FAILED)
    {
      log_e("Сканування не було запущено");
      return out_vector;
    }
    else if (scan_result < 0)
    {
      log_e("Помилка отримання результатів сканування: %d", scan_result);
      return out_vector;
    }

    if (scan_result > 0)
    {
      out_vector.reserve(static_cast<size_t>(scan_result));
      for (int16_t i{0}; i < scan_result; ++i)
        out_vector.emplace_back(WiFi.SSID(i));
    }

    WiFi.scanDelete();
    return out_vector;
  }

  bool WiFiManager::setPower(const WiFiPowerLevel power_lvl)
  {
    bool result{false};
    switch (power_lvl)
    {
      case WIFI_POWER_MIN:
        result = WiFi.setTxPower(WIFI_POWER_5dBm);
        break;
      case WIFI_POWER_MEDIUM:
        result = WiFi.setTxPower(WIFI_POWER_15dBm);
        break;
      case WIFI_POWER_MAX:
        result = WiFi.setTxPower(WIFI_POWER_19_5dBm);
        break;
      default:
        log_e("Отримано некоректний рівень потужності WiFi");
        result = false;
        break;
    }

    return result;
  }

  bool WiFiManager::savePower(const WiFiPowerLevel power_lvl)
  {
    if (power_lvl > WIFI_POWER_MAX)
    {
      log_e("Отримано некоректний рівень потужності WiFi");
      return false;
    }

    if (isEnabled())
      setPower(power_lvl);

    return savePowerSettings(power_lvl);
  }

  WiFiManager::WiFiPowerLevel WiFiManager::getPower() const
  {
    const wifi_power_t power = WiFi.getTxPower();

    if (power > WIFI_POWER_15dBm)
      return WIFI_POWER_MAX;

    if (power > WIFI_POWER_5dBm)
      return WIFI_POWER_MEDIUM;

    return WIFI_POWER_MIN;
  }

  WiFiManager::WiFiPowerLevel WiFiManager::readPowerSettings() const
  {
    const String wifi_power = SettingsManager::get(STR_PREF_WIFI_POWER, STR_WIFI_SUBDIR);

    if (wifi_power.isEmpty())
    {
      log_e("Помилка зчитування налаштувань потужності WiFi");
      return WIFI_POWER_MIN;
    }

    const int power = std::atoi(wifi_power.c_str());
    if (power < WIFI_POWER_MIN || power > WIFI_POWER_MAX)
    {
      log_e("Некоректне значення потужності WiFi у налаштуваннях: %d", power);
      return WIFI_POWER_MIN;
    }

    return static_cast<WiFiPowerLevel>(power);
  }

  bool WiFiManager::savePowerSettings(const WiFiPowerLevel power_lvl) const
  {
    return SettingsManager::set(STR_PREF_WIFI_POWER, String(power_lvl), STR_WIFI_SUBDIR);
  }

  bool WiFiManager::isConnected() const
  {
    return WiFi.status() == WL_CONNECTED;
  }

  bool WiFiManager::isApEnabled() const
  {
    return (WiFi.getMode() & WIFI_MODE_AP) != 0;
  }

  String WiFiManager::getSSID() const
  {
    if (isConnected())
      return WiFi.SSID();

    return emptyString;
  }

  void WiFiManager::disconnect()
  {
    resetOpState(OP_CONNECTING);
    WiFi.disconnect(false);
    delay(100);
  }

  bool WiFiManager::isEnabled() const
  {
    return WiFi.getMode() != WIFI_MODE_NULL;
  }

  bool WiFiManager::enable()
  {
    if (!ensureEventsRegistered())
      return false;

    if (WiFi.getMode() == WIFI_MODE_STA)
      return true;

    if (isApEnabled())
      WiFi.softAPdisconnect(false);

    WiFi.persistent(false);
    const bool result = WiFi.mode(WIFI_MODE_STA);
    if (result)
      setPower(readPowerSettings());

    return result;
  }

  void WiFiManager::disable()
  {
    OperationState prev_state{OP_IDLE};

    if (_mutex && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE)
    {
      prev_state = _op_state;
      _op_state = OP_IDLE;
      xSemaphoreGive(_mutex);
    }

    if (prev_state == OP_SCANNING)
      esp_wifi_scan_stop();

    WiFi.setAutoReconnect(false);

    if (isApEnabled())
      WiFi.softAPdisconnect(false);

    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
    WiFi.scanDelete();
  }

  bool WiFiManager::toggle()
  {
    if (isEnabled())
    {
      disable();
      return true;
    }

    recoverIfTimedOut();

    if (isBusy())
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    return enable();
  }

  String WiFiManager::getIP() const
  {
    if (isApEnabled())
      return WiFi.softAPIP().toString();
    else if (isConnected())
      return WiFi.localIP().toString();
    else
      return emptyString;
  }

  bool WiFiManager::isBusy() const
  {
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return false;

    bool busy = _op_state != OP_IDLE;
    if (busy && (_op_state == OP_SCANNING || _op_state == OP_CONNECTING) && (millis() - _op_start_ms) >= _op_timeout_ms)
      busy = false;

    xSemaphoreGive(_mutex);
    return busy;
  }

  void WiFiManager::handleConnDone(const wl_status_t conn_status)
  {
    ConnectCompleteHandler handler{nullptr};
    void* arg{nullptr};
    String ssid;

    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    if (_op_state != OP_CONNECTING)
    {
      xSemaphoreGive(_mutex);
      return;
    }

    _op_state = OP_IDLE;
    handler = _connect_complete_handler;
    arg = _connect_complete_handler_arg;
    ssid = _last_ssid;
    xSemaphoreGive(_mutex);

    log_i("WiFi.status: %d", conn_status);

    if (handler)
      handler(arg, ssid, conn_status);
  }

  void WiFiManager::handleScanDone()
  {
    ScanCompleteHandler handler{nullptr};
    void* arg{nullptr};

    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
      return;

    if (_op_state != OP_SCANNING)
    {
      xSemaphoreGive(_mutex);
      return;
    }

    _op_state = OP_IDLE;
    handler = _scan_complete_handler;
    arg = _scan_complete_handler_arg;
    xSemaphoreGive(_mutex);

    if (handler)
      handler(arg);
    else
      WiFi.scanDelete();
  }

  void WiFiManager::onEvent(const WiFiEvent_t event)
  {
    switch (event)
    {
      case ARDUINO_EVENT_WIFI_SCAN_DONE:
        _wifi.handleScanDone();
        break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _wifi.handleConnDone(WL_CONNECTED);
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      {
        const wl_status_t status = WiFi.status();
        _wifi.handleConnDone(status == WL_CONNECTED ? WL_DISCONNECTED : status);
        break;
      }
      default:
        break;
    }
  }

  WiFiManager _wifi;
}  // namespace pixeler
