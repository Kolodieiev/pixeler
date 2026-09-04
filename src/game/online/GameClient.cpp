#pragma GCC optimize("O3")
#include "GameClient.h"

#include "manager/WiFiManager.h"

namespace pixeler
{
#ifndef GAME_SERVER_PORT
#define GAME_SERVER_PORT 777
#endif  // #ifndef GAME_SERVER_PORT

#ifndef CLIENT_PACKS_QUEUE_SIZE
#define CLIENT_PACKS_QUEUE_SIZE 16
#endif  // #ifndef CLIENT_PACKS_QUEUE_SIZE

#ifndef GAME_CLIENT_PORT
#define GAME_CLIENT_PORT 333
#endif  // #ifndef GAME_CLIENT_PORT

  GameClient::GameClient()
  {
    // Виправлення помилки assert failed: tcpip_api_call (Invalid mbox)
    if (!_wifi.isEnabled())
      _wifi.enable();
  }

  GameClient::~GameClient()
  {
    disconnect();
  }

  bool GameClient::connect(const String& client_name, const String& game_ID, const String& host_IP)
  {
    if (client_name.isEmpty())
    {
      log_e("Не вказано ім'я клієнта");
      return false;
    }

    if (game_ID.isEmpty())
    {
      log_e("Не вказано game_ID");
      return false;
    }

    if (!_server_ip.fromString(host_IP))
    {
      log_e("Некоректний IP сервера: %s", host_IP);
      return false;
    }

    _login = client_name;
    _game_id = game_ID;

    if (!_wifi.isConnected())
    {
      log_e("%s", STR_ROUTER_NOT_CONNECTED);
      return false;
    }

    log_i("Приєднання до сервера...");
    _status = STATUS_IDLE;

    _client.onPacket(onPacket, this);
    _client.listen(GAME_CLIENT_PORT);

    _last_act_time = millis();

    _udp_mutex = xSemaphoreCreateMutex();
    if (!_udp_mutex)
    {
      log_e("Не вдалося створити _udp_mutex");
      esp_restart();
    }

    _packet_queue = xQueueCreate(CLIENT_PACKS_QUEUE_SIZE, sizeof(UdpPacket*));
    if (!_packet_queue)
    {
      log_e("Не вдалося створити _packet_queue");
      esp_restart();
    }

    xTaskCreatePinnedToCore(checkConnectTask, "checkConnectTask", (1024 / 2) * 4, this, 10, &_check_task_handler, 1);
    xTaskCreatePinnedToCore(packetHandlerTask, "packetHandlerTask", (1024 / 2) * 10, this, 10, &_packet_task_handler, 1);

    if (!_check_task_handler)
    {
      log_e("Не вдалося запустити checkConnectTask");
      esp_restart();
    }

    if (!_packet_task_handler)
    {
      log_e("Не вдалося запустити packetHandlerTask");
      esp_restart();
    }

    sendHandshake();

    _is_freed = false;
    return true;
  }

  void GameClient::disconnect()
  {
    if (_is_freed)
      return;

    _status = STATUS_DISCONNECTED;

    _is_freed = true;

    log_i("Від'єднано від сервера");

    _client.close();

    if (_check_task_handler)
    {
      vTaskDelete(_check_task_handler);
      _check_task_handler = nullptr;
    }

    if (_packet_task_handler)
    {
      vTaskDelete(_packet_task_handler);
      _packet_task_handler = nullptr;
    }

    if (_packet_queue)
    {
      UdpPacket* packet{nullptr};
      while (xQueueReceive(_packet_queue, &packet, 0) == pdPASS)
        delete packet;

      vQueueDelete(_packet_queue);
      _packet_queue = nullptr;
    }

    if (_udp_mutex)
    {
      vSemaphoreDelete(_udp_mutex);
      _udp_mutex = nullptr;
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::sendPacket(const UdpPacket& packet)
  {
    if (_status != STATUS_CONNECTED && _status != STATUS_IDLE) [[unlikely]]
    {
      log_e("Відсутнє з'єднання з сервером");
      return;
    }

    xSemaphoreTake(_udp_mutex, portMAX_DELAY);
    _client.writeTo(packet.raw(), packet.length(), _server_ip, GAME_SERVER_PORT);
    xSemaphoreGive(_udp_mutex);
  }

  void GameClient::send(UdpPacket::PacketType type, uint8_t subtype, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.setSubtype(subtype);
    pack.write(data, data_size);
    sendPacket(pack);
  }

  GameClient::Status GameClient::getStatus() const
  {
    return _status;
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::sendHandshake()
  {
    log_i("Рукостискання...");

    UdpPacket packet(_game_id.length());
    packet.setType(UdpPacket::TYPE_CONNECT);
    packet.setSubtype(UdpPacket::SUBTYPE_HANDSHAKE);
    packet.write(_game_id.c_str(), _game_id.length());

    sendPacket(packet);
  }

  void GameClient::sendLogin()
  {
    log_i("Авторизація...");

    UdpPacket packet(_login.length());
    packet.setType(UdpPacket::TYPE_CONNECT);
    packet.setSubtype(UdpPacket::SUBTYPE_LOGIN);
    packet.write(_login.c_str(), _login.length());

    sendPacket(packet);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::handlePacket(const UdpPacket& packet)
  {
    switch (packet.getType())
    {
      case UdpPacket::TYPE_DATA:
        invokeDataHandler(packet);
        break;
      case UdpPacket::TYPE_PING:
        handlePing();
        break;
      case UdpPacket::TYPE_CONNECT:
        handleConnect(packet);
        break;
      case UdpPacket::TYPE_CLIENT_DATA:
        handleClientData(packet);
        break;
      default:
        log_e("Неочікуваний тип пакета:");
        if (CORE_DEBUG_LEVEL > 0)
          packet.printToLog();
        break;
    }
  }

  void GameClient::packetHandlerTask(void* arg)
  {
    GameClient* self{static_cast<GameClient*>(arg)};
    UdpPacket* packet{nullptr};
    uint32_t processed_count{0};

    while (1)
    {
      if (xQueueReceive(self->_packet_queue, &packet, portMAX_DELAY) == pdPASS)
      {
        self->handlePacket(*packet);
        delete packet;

        if ((++processed_count & 15) == 0)
          delay(1);
      }
    }
  }

  void GameClient::onPacket(void* arg, AsyncUDPPacket& packet)
  {
    size_t packet_len = packet.length();
    if (packet_len > 1000 || packet_len < 2)
    {
      log_e("Некоректний розмір пакета: %zu", packet_len);
      return;
    }

    GameClient* self = static_cast<GameClient*>(arg);

    if (self->_packet_queue)
    {
      UdpPacket* pack = new UdpPacket(packet);

      if (!xQueueSend(self->_packet_queue, &pack, 0) == pdPASS)
      {
        log_e("Черга _packet_queue переповнена");
        delete pack;
      }
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::handleConnect(const UdpPacket& packet)
  {
    uint8_t subtype = packet.getSubtype();

    switch (subtype)
    {
      case UdpPacket::SUBTYPE_HANDSHAKE:
        log_i("Сервер гри розпізнано");
        sendLogin();
        break;

      case UdpPacket::SUBTYPE_INCORRECT_SERVER:
        log_i("Некоректний сервер гри");
        invokeErrorHandler(ERR_INCORRECT_SERVER);
        disconnect();
        break;

      case UdpPacket::SUBTYPE_ACCESS_GRANTED:
        log_i("Приєднано до сервера");
        _status = STATUS_CONNECTED;
        invokeConnectHandler();
        break;

      case UdpPacket::SUBTYPE_ACCESS_DENIED:
        log_i("Приєднання відхилено сервером");
        invokeErrorHandler(ERR_ACCESS_DENIED);
        disconnect();
        break;

      case UdpPacket::SUBTYPE_INCORRECT_NAME:
        log_i("Некоректне ім'я клієнта");
        invokeErrorHandler(ERR_INCORRECT_NAME);
        disconnect();
        break;

      case UdpPacket::SUBTYPE_BUSY:
        log_i("Сервер зайнятий");
        invokeErrorHandler(ERR_SERVER_BUSY);
        disconnect();
        break;

      default:
        log_e("Некоректний підтип пакету підключення: %u", subtype);
        break;
    }
  }

  void GameClient::handleClientData(const UdpPacket& packet)
  {
    uint8_t subtype = packet.getSubtype();

    switch (subtype)
    {
      case UdpPacket::SUBTYPE_START_GAME:
        log_i("Гра розпочинаєтсья");
        invokeGameStartHandler();
        break;

      default:
        log_e("Некоректний підтип пакету клієнтських даних: %u", subtype);
        break;
    }
  }

  void GameClient::handlePing()
  {
    _last_act_time = millis();

    UdpPacket packet;
    packet.setType(UdpPacket::TYPE_PING);

    sendPacket(packet);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::handleCheckConnect()
  {
    if (millis() - _last_act_time > 3000) [[unlikely]]
    {
      log_i("З'єднання з сервером втрачено");
      invokeDisconnectHandler();
      disconnect();
    }
  }

  void GameClient::checkConnectTask(void* arg)
  {
    GameClient* self = static_cast<GameClient*>(arg);

    while (1)
    {
      self->handleCheckConnect();
      delay(2000);
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::invokeDataHandler(const UdpPacket& packet)
  {
    if (!_data_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник даних від сервера");
      return;
    }

    _data_handler(packet, _data_arg);
  }

  void GameClient::invokeConnectHandler()
  {
    if (!_connect_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник підключення до сервера");
      return;
    }

    log_i("Викликаю connect_handler");
    _connect_handler(_connect_arg);
  }

  void GameClient::invokeDisconnectHandler()
  {
    if (!_disconnect_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник відключення від сервера");
      return;
    }

    log_i("Викликаю disconnect_handler");
    _disconnect_handler(_disconnect_arg);
  }

  void GameClient::invokeGameStartHandler()
  {
    if (!_game_start_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник старту гри");
      return;
    }

    log_i("Викликаю game_start_handler");
    _game_start_handler(_game_start_arg);
  }

  void GameClient::invokeErrorHandler(Error error)
  {
    if (!_error_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник помилок клієнта");
      return;
    }

    log_i("Викликаю error_handler");
    _error_handler(error, _error_arg);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::onData(DataHandler handler, void* arg)
  {
    _data_handler = handler;
    _data_arg = arg;
  }

  void GameClient::onConnect(ConnectHandler handler, void* arg)
  {
    _connect_handler = handler;
    _connect_arg = arg;
  }

  void GameClient::onGameStart(GameStartHandler handler, void* arg)
  {
    _game_start_handler = handler;
    _game_start_arg = arg;
  }

  void GameClient::OnError(ErrorHandler handler, void* arg)
  {
    _error_handler = handler;
    _error_arg = arg;
  }

  void GameClient::onDisconnect(DisconnectHandler handler, void* arg)
  {
    _disconnect_handler = handler;
    _disconnect_arg = arg;
  }
}  // namespace pixeler
