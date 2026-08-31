#pragma GCC optimize("O3")
#include "WiFiManager.h"

#include "SettingsManager.h"

namespace pixeler
{
  static const char STR_PREF_WIFI_POWER[] = "wifipower";

  static const char STR_ERR_WIFI_BUSY[] = "WiFi-модуль зайнятий";
  static const char STR_ERR_EMPTY_SSID[] = "SSID не може бути порожній";
  static const char STR_ERR_UNKNOWN_SSID[] = "Невідомий SSID:";

  bool WiFiManager::tryConnectTo(const String& ssid, const String& pwd, uint8_t wifi_chan, bool autoreconnect)
  {
    if (_is_busy)
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return false;
    }

    if (isConnected())
      disconnect();

    if (wifi_chan > 10)
      wifi_chan = 10;

    WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.setAutoReconnect(autoreconnect);
    WiFi.persistent(false);
    wl_status_t status = WiFi.begin(ssid, pwd, wifi_chan);

    if (status != WL_DISCONNECTED)
    {
      log_e("Помилка приєднання до: %s", ssid.c_str());
      WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
      WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);
      return false;
    }

    _is_busy = true;
    return true;
  }

  bool WiFiManager::tryConnectToKnown(const String& ssid, uint8_t wifi_chan, bool autoreconnect)
  {
    String pwd = getSSIDKey(ssid);
    if (pwd.isEmpty())
    {
      log_e("%s %s", STR_ERR_UNKNOWN_SSID, ssid.c_str());
      return false;
    }

    return tryConnectTo(ssid, pwd, wifi_chan, autoreconnect);
  }

  bool WiFiManager::saveSSID(const String& ssid, const String& pwd) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return emptyString;
    }

    return SettingsManager::set(ssid, pwd, STR_WIFI_SUBDIR);
  }

  bool WiFiManager::forgetSSID(const String& ssid) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return emptyString;
    }

    String ssid_keys_path = SettingsManager::getSettingsFilePath(ssid, STR_WIFI_SUBDIR);
    if (ssid_keys_path.isEmpty())
    {
      log_e("%s %s", STR_ERR_UNKNOWN_SSID, ssid.c_str());
      return false;
    }

    return _fs.rmFile(ssid_keys_path.c_str());
  }

  bool WiFiManager::hasKnownSSID(const String& ssid) const
  {
    if (ssid.isEmpty())
    {
      log_e("%s", STR_ERR_EMPTY_SSID);
      return emptyString;
    }

    String pwd = SettingsManager::get(ssid, STR_WIFI_SUBDIR);
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
      log_e("Невідомий SSID: %s", ssid.c_str());

    return pwd;
  }

  bool WiFiManager::createAP(const String& ssid, const String& pwd, uint8_t max_connection, uint8_t wifi_chan, bool is_hidden)
  {
    if (_is_busy)
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    if (isConnected())
      disconnect();

    if (max_connection > 9)
      max_connection = 9;

    if (max_connection == 0)
      max_connection = 1;

    if (wifi_chan > 10)
      wifi_chan = 10;

    bool result = WiFi.softAP(ssid, pwd, wifi_chan, is_hidden, max_connection);
    delay(100);

    if (!result)
      log_e("Помилка створення точки доступу");

    return result;
  }

  void WiFiManager::onConnectComplete(ConnectCompleteHandler handler, void* arg)
  {
    _connect_complete_handler = handler;
    _connect_complete_handler_arg = arg;
  }

  void WiFiManager::onScanComplete(ScanCompleteHandler handler, void* arg)
  {
    _scan_complete_handler = handler;
    _scan_complete_handler_arg = arg;
  }

  bool WiFiManager::startScan()
  {
    if (_is_busy)
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return false;
    }

    if (!isEnabled() && !enable())
      return false;
    else if (WiFi.getMode() != WIFI_MODE_STA)
      enable();

    WiFi.onEvent(onEvent, ARDUINO_EVENT_WIFI_SCAN_DONE);
    int16_t result_code = WiFi.scanNetworks(true);
    if (result_code == WIFI_SCAN_FAILED)
    {
      log_e("Помилка запуску сканера Wi-Fi");
      WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_SCAN_DONE);
      return false;
    }
    else
    {
      _is_busy = true;
    }

    return true;
  }

  std::vector<String> WiFiManager::getScanResult()
  {
    std::vector<String> out_vector;

    if (_is_busy)
    {
      log_e("%s", STR_ERR_WIFI_BUSY);
      return out_vector;
    }

    int16_t scan_result = WiFi.scanComplete();

    if (scan_result == -1)
    {
      log_e("Сканування ще не завершено");
      return out_vector;
    }
    else if (scan_result == -2)
    {
      log_e("Сканування не було запущено");
      return out_vector;
    }

    out_vector.reserve(scan_result);

    for (uint16_t i = 0; i < scan_result; ++i)
      out_vector.emplace_back(WiFi.SSID(i));

    WiFi.scanDelete();

    return out_vector;
  }

  bool WiFiManager::setPower(WiFiPowerLevel power_lvl)
  {
    bool result = false;
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

  bool WiFiManager::savePower(WiFiPowerLevel power_lvl)
  {
    setPower(power_lvl);
    return savePowerSettings(power_lvl);
  }

  WiFiManager::WiFiPowerLevel WiFiManager::getPower() const
  {
    wifi_power_t power = WiFi.getTxPower();

    if (power > WIFI_POWER_15dBm)
      return WIFI_POWER_MAX;

    if (power > WIFI_POWER_5dBm)
      return WIFI_POWER_MEDIUM;

    return WIFI_POWER_MIN;
  }

  WiFiManager::WiFiPowerLevel WiFiManager::readPowerSettings() const
  {
    String wifi_power = SettingsManager::get(STR_PREF_WIFI_POWER, STR_WIFI_SUBDIR);

    if (wifi_power.isEmpty())
    {
      log_e("Помилка зчитування налаштувань потужності WiFi");
      return WIFI_POWER_MIN;
    }

    int power = std::atoi(wifi_power.c_str());
    return static_cast<WiFiManager::WiFiPowerLevel>(power);
  }

  bool WiFiManager::savePowerSettings(WiFiPowerLevel power_lvl) const
  {
    return SettingsManager::set(STR_PREF_WIFI_POWER, String(power_lvl).c_str(), STR_WIFI_SUBDIR);
  }

  bool WiFiManager::isConnected() const
  {
    return WiFi.status() == WL_CONNECTED;
  }

  bool WiFiManager::isApEnabled() const
  {
    return WiFi.getMode() & WIFI_AP;
  }

  String WiFiManager::getSSID() const
  {
    if (isConnected())
      return WiFi.SSID();
    else
      return emptyString;
  }

  void WiFiManager::disconnect()
  {
    _connect_complete_handler = nullptr;
    _scan_complete_handler = nullptr;
    WiFi.disconnect();
    delay(100);
  }

  bool WiFiManager::isEnabled() const
  {
    return WiFi.getMode() != WIFI_MODE_NULL;
  }

  bool WiFiManager::enable()
  {
    bool result = WiFi.mode(WIFI_MODE_STA);
    if (result)
    {
      WiFiPowerLevel power = readPowerSettings();
      setPower(power);
    }

    return result;
  }

  void WiFiManager::disable()
  {
    disconnect();
    WiFi.mode(WIFI_OFF);
  }

  bool WiFiManager::toggle()
  {
    if (_is_busy)
    {
      log_e("Модуль зайнятий");
      return false;
    }

    if (isEnabled())
    {
      disable();
      return true;
    }
    else
    {
      return enable();
    }
  }

  String WiFiManager::getIP()
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
    return _is_busy;
  }

  void WiFiManager::invokeConnDoneHandler()
  {
    log_i("WiFi.status: %d", WiFi.status());

    if (_connect_complete_handler)
      _connect_complete_handler(_connect_complete_handler_arg, WiFi.status());
  }

  void WiFiManager::invokeScanDoneHandler()
  {
    if (_scan_complete_handler)
      _scan_complete_handler(_scan_complete_handler_arg);
    else
      WiFi.scanDelete();
  }

  void WiFiManager::onEvent(WiFiEvent_t event)
  {
    switch (event)
    {
      case ARDUINO_EVENT_WIFI_SCAN_DONE:
        WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_SCAN_DONE);
        _wifi._is_busy = false;
        _wifi.invokeScanDoneHandler();
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        WiFi.removeEvent(onEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);

        if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
        {
          long unsigned got_ip_time = millis();
          while (millis() - got_ip_time < 2000 && WiFi.status() != WL_CONNECTED)
            delay(50);
        }
        _wifi._is_busy = false;
        _wifi.invokeConnDoneHandler();
        break;
      default:
        log_e("Unknown wifi event: %u", event);
        break;
    }
  }

  WiFiManager _wifi;
}  // namespace pixeler
