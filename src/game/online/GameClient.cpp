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

    _nickname = client_name;
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

    _is_freed = true;

    log_i("Від'єднано від сервера");

    _data_handler = nullptr;
    _connect_handler = nullptr;
    _disconnect_handler = nullptr;

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
      while (xQueueReceive(_packet_queue, &packet, portMAX_DELAY) == pdPASS)
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

  void GameClient::send(UdpPacket::PacketType type, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.write(data, data_size);
    sendPacket(pack);
  }

  GameClient::ClientStatus GameClient::getStatus() const
  {
    return _status;
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::sendHandshake()
  {
    log_i("Рукостискання...");

    UdpPacket packet(_game_id.length());
    packet.setType(UdpPacket::TYPE_HANDSHAKE);
    packet.write(_game_id.c_str(), _game_id.length());

    sendPacket(packet);
  }

  void GameClient::sendName()
  {
    log_i("Авторизація...");

    UdpPacket packet(_nickname.length());
    packet.setType(UdpPacket::TYPE_NAME);
    packet.write(_nickname.c_str(), _nickname.length());

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
      case UdpPacket::TYPE_NAME:
        handleNameConfirm(packet);
        break;
      case UdpPacket::TYPE_HANDSHAKE:
        handleHandshake(packet);
        break;
      case UdpPacket::TYPE_BUSY:
        handleBusy();
        break;
      case UdpPacket::TYPE_NAME_INCORRECT:
        handleIncorrectName();
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
    if (packet.length() > 1000 || packet.length() == 0)
    {
      log_e("Некоректний пакет");
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

  void GameClient::handleHandshake(const UdpPacket& packet)
  {
    if (static_cast<uint8_t>(packet.getData()[0]) != 1)
    {
      log_e("Некоректний сервер");
      _status = STATUS_INCORRECT_SERVER;
      disconnect();
      invokeDisconnectHandler();
    }
    else
    {
      log_i("Сервер розпізнано");
      sendName();
    }
  }

  void GameClient::handleNameConfirm(const UdpPacket& packet)
  {
    if (static_cast<uint8_t>(packet.getData()[0]) != 1)
    {
      log_e("Приєднання відхилено сервером");
      _status = STATUS_ACCESS_DENIED;
      disconnect();
      invokeDisconnectHandler();
    }
    else
    {
      log_i("Приєднано до сервера");
      _status = STATUS_CONNECTED;
      invokeConnectHandler();
    }
  }

  void GameClient::handleIncorrectName()
  {
    log_e("Некоректне ім'я клієнта");
    _status = STATUS_INCORRECT_NAME;
    disconnect();
    invokeDisconnectHandler();
  }

  void GameClient::handlePing()
  {
    _last_act_time = millis();

    UdpPacket packet(1);
    packet.setType(UdpPacket::TYPE_PING);

    sendPacket(packet);
  }

  void GameClient::handleBusy()
  {
    log_e("Сервер зайнятий");
    _status = STATUS_SERVER_BUSY;
    disconnect();
    invokeDisconnectHandler();
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::handleCheckConnect()
  {
    if (millis() - _last_act_time > 3000) [[unlikely]]
    {
      log_e("З'єднання з сервером втрачено");
      _status = STATUS_DISCONNECTED;
      disconnect();
      invokeDisconnectHandler();
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

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameClient::onData(ServerDataHandler data_handler, void* arg)
  {
    _data_handler = data_handler;
    _data_arg = arg;
  }

  void GameClient::onConnect(ServerConnectHandler handler, void* arg)
  {
    _connect_handler = handler;
    _connect_arg = arg;
  }

  void GameClient::onDisconnect(ServerDisconnectHandler handler, void* arg)
  {
    _disconnect_handler = handler;
    _disconnect_arg = arg;
  }
}  // namespace pixeler
