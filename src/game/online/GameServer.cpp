#pragma GCC optimize("O3")
#include "GameServer.h"

#include "manager/WiFiManager.h"

namespace pixeler
{

#ifndef GAME_SERVER_PORT
#define GAME_SERVER_PORT 777
#endif  // #ifndef GAME_SERVER_PORT

#ifndef SERVER_PACKS_QUEUE_SIZE
#define SERVER_PACKS_QUEUE_SIZE 32
#endif  // #ifndef SERVER_PACKS_QUEUE_SIZE

  GameServer::GameServer()
  {
    // Виправлення помилки assert failed: tcpip_api_call (Invalid mbox)
    if (!_wifi.isEnabled())
      _wifi.enable();
  }

  GameServer::~GameServer()
  {
    stop();
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  bool GameServer::begin(const String& game_ID, const String& server_name, const String& pwd, bool is_local, uint8_t max_connection, uint8_t wifi_chan)
  {
    _game_id = game_ID;
    _max_connection = max_connection;

    if (is_local)
    {
      String serv_name{server_name};
      String password{pwd};
      if (!_wifi.createAP(serv_name, password, _max_connection, wifi_chan))
        return false;
    }
    else if (!_wifi.isConnected())
    {
      log_e("%s", STR_ROUTER_NOT_CONNECTED);
      return false;
    }

    _server_ip = "http://";
    _server_ip += _wifi.getIP();

    log_i("Game server address: %s", _server_ip.c_str());

    _server_name = server_name;

    _server.onPacket(onPacket, this);
    _server.listen(GAME_SERVER_PORT);

    _client_mutex = xSemaphoreCreateMutex();
    _udp_mutex = xSemaphoreCreateMutex();

    if (!_client_mutex)
    {
      log_e("Не вдалося створити _client_mutex");
      esp_restart();
    }

    if (!_udp_mutex)
    {
      log_e("Не вдалося створити _udp_mutex");
      esp_restart();
    }

    _packet_queue = xQueueCreate(SERVER_PACKS_QUEUE_SIZE, sizeof(UdpPacket*));

    if (!_packet_queue)
    {
      log_e("Не вдалося створити _packet_queue");
      esp_restart();
    }

    xTaskCreatePinnedToCore(pingClientsTask, "pingCl", (1024 / 2) * 4, this, 10, &_ping_task_handler, 1);
    xTaskCreatePinnedToCore(packetHandlerTask, "packHndl", (1024 / 2) * 10, this, 10, &_packet_task_handler, 1);

    if (!_ping_task_handler)
    {
      log_e("Не вдалося запустити pingClientsTask");
      esp_restart();
    }

    if (!_packet_task_handler)
    {
      log_e("Не вдалося запустити packetHandlerTask");
      esp_restart();
    }

    _is_freed = false;
    return true;
  }

  void GameServer::stop()
  {
    if (_is_freed)
      return;

    _is_freed = true;

    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    _data_handler = nullptr;
    _confirmation_handler = nullptr;
    _disconnect_handler = nullptr;

    _server.close();
    _clients.clear();

    xSemaphoreGive(_client_mutex);

    if (_ping_task_handler)
    {
      vTaskDelete(_ping_task_handler);
      _ping_task_handler = nullptr;
    }

    if (_packet_task_handler)
    {
      vTaskDelete(_packet_task_handler);
      _packet_task_handler = nullptr;
    }

    if (_client_mutex)
    {
      vSemaphoreDelete(_client_mutex);
      _client_mutex = nullptr;
    }

    if (_udp_mutex)
    {
      vSemaphoreDelete(_udp_mutex);
      _udp_mutex = nullptr;
    }

    if (_packet_queue)
    {
      UdpPacket* packet{nullptr};
      while (xQueueReceive(_packet_queue, &packet, 0) == pdPASS)
        delete packet;

      vQueueDelete(_packet_queue);
      _packet_queue = nullptr;
    }

    _wifi.disable();
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::open()
  {
    _is_open = true;
    log_i("Сервер відкрито");
  }

  void GameServer::close()
  {
    _is_open = false;
    xSemaphoreTake(_client_mutex, portMAX_DELAY);
    for (auto it = _clients.begin(), last_it = _clients.end(); it != last_it;)
    {
      if (!it->second.isConfirmed())
      {
        it = _clients.erase(it);
        last_it = _clients.end();
      }
      else
      {
        ++it;
      }
    }
    xSemaphoreGive(_client_mutex);
    log_i("Сервер закрито");
  }

  void GameServer::toggle()
  {
    if (_is_open)
      close();
    else
      open();
  }

  bool GameServer::isOpen() const
  {
    return _is_open;
  }

  bool GameServer::isFull() const
  {
    return _max_connection == _confirmed_clients_num;
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::sendBroadcast(UdpPacket::PacketType type, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.write(data, data_size);
    sendBroadcast(pack);
  }

  void GameServer::sendBroadcast(const UdpPacket& packet)
  {
    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    for (auto it = _clients.begin(), last_it = _clients.end(); it != last_it; ++it)
      sendPacket(it->second, packet);

    xSemaphoreGive(_client_mutex);
  }

  void GameServer::sendPacket(const ClientSession& client, UdpPacket::PacketType type, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.write(data, data_size);

    sendPacket(client, pack);
  }

  void GameServer::sendPacket(const ClientSession& client, const UdpPacket& packet)
  {
    xSemaphoreTake(_udp_mutex, portMAX_DELAY);
    _server.writeTo(packet.raw(), packet.length(), client.getIP(), client.getPort());
    xSemaphoreGive(_udp_mutex);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::removeClient(const String& client_name)
  {
    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    for (auto it = _clients.begin(), last_it = _clients.end(); it != last_it; ++it)
    {
      if (it->second.hasName(client_name))
      {
        _clients.erase(it);
        break;
      }
    }

    xSemaphoreGive(_client_mutex);
  }

  void GameServer::removeClient(IPAddress remote_ip)
  {
    uint32_t cl_ip = remote_ip;
    if (cl_ip == 0)
      return;

    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    auto it = _clients.find(remote_ip);
    if (it != _clients.end())
      _clients.erase(it);

    xSemaphoreGive(_client_mutex);
  }

  ClientSession* GameServer::findClient(IPAddress remote_ip)
  {
    uint32_t cl_ip = remote_ip;
    if (cl_ip == 0)
      return nullptr;

    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    auto it = _clients.find(remote_ip);

    if (it == _clients.end())
    {
      xSemaphoreGive(_client_mutex);
      return nullptr;
    }

    xSemaphoreGive(_client_mutex);
    return &it->second;
  }

  ClientSession* GameServer::findClient(const String& name)
  {
    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    for (auto it = _clients.begin(), last_it = _clients.end(); it != last_it; ++it)
      if (it->second.hasName(name))
      {
        xSemaphoreGive(_client_mutex);
        return &it->second;
      }

    xSemaphoreGive(_client_mutex);
    return nullptr;
  }

  const ClientSession* GameServer::findClient(IPAddress remote_ip) const
  {
    return findClient(remote_ip);
  }

  const ClientSession* GameServer::findClient(const String& name) const
  {
    return findClient(name);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::sendNameRespMsg(const ClientSession& client, bool result)
  {
    if (!result)
      log_i("Авторизацію відхилено: %s", client.getName());
    else
      log_i("Авторизовано: %s", client.getName());

    uint8_t resp = result;
    UdpPacket packet(sizeof(resp));
    packet.setType(UdpPacket::TYPE_NAME);
    packet.write(&resp, sizeof(resp));

    sendPacket(client, packet);
  }

  void GameServer::sendNameIncorrectMsg(const ClientSession& client)
  {
    log_i("Некоректне ім'я клієнта");

    uint8_t data = 1;
    UdpPacket packet(sizeof(data));
    packet.setType(UdpPacket::TYPE_NAME_INCORRECT);
    packet.write(&data, sizeof(data));

    sendPacket(client, packet);
  }

  void GameServer::sendBusyMsg(const ClientSession& client)
  {
    log_i("Сервер зайнятий");

    uint8_t data = 1;
    UdpPacket packet(sizeof(data));
    packet.setType(UdpPacket::TYPE_BUSY);
    packet.write(&data, sizeof(data));

    sendPacket(client, packet);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::handleHandshake(const UdpPacket& packet)
  {
    uint8_t result = packet.isDataEquals(_game_id.c_str());

    UdpPacket resp_msg{sizeof(result)};
    resp_msg.setType(UdpPacket::TYPE_HANDSHAKE);
    resp_msg.write(&result, sizeof(result));
    _server.writeTo(resp_msg.raw(), resp_msg.length(), packet.getRemoteIP(), packet.getRemotePort());
  }

  void GameServer::handleName(ClientSession& client, const UdpPacket& packet)
  {
    log_i("Запит авторизації");

    if (client.isConfirmed())
    {
      /* Помилкова повторна авторизація вже авторизованого клієнта.
       * Мусить дочекатися видалення з сервера через 3 сек. */
      sendNameRespMsg(client, false);
      return;
    }

    if (packet.dataLen() > 20 || _server_name.equals(packet.getData()) || findClient((const String&)packet.getData()))
    {
      sendNameIncorrectMsg(client);
      removeClient(client.getIP());
      return;
    }

    if (_is_busy)
    {
      sendBusyMsg(client);
      removeClient(client.getIP());
      return;
    }

    /* Якщо сервер вже приймає рішення щодо авторизації клієнта,
     * інші клієнти не повинні переривати розгляд поточного рішення. */
    _is_busy = true;

    client.setName(packet.getData());
    invokeConfirmationHandler(client, onConfirmationResult);
  }

  void GameServer::handleData(const ClientSession& client, const UdpPacket& packet)
  {
    if (!client.isConfirmed())
    {
      removeClient(client.getIP());
      return;
    }

    if (!_data_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник даних від клієнтів");
      return;
    }

    _data_handler(client, packet, _data_arg);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::handlePacket(const UdpPacket& packet)
  {
    UdpPacket::PacketType type = packet.getType();
    ClientSession* client = findClient(packet.getRemoteIP());

    if (client)
    {
      switch (type)
      {
        case UdpPacket::TYPE_DATA:
          handleData(*client, packet);
          break;
        case UdpPacket::UdpPacket::TYPE_PING:
          client->prolong();
          break;
        case UdpPacket::TYPE_NAME:
          handleName(*client, packet);
          break;
        default:
          log_e("Неочікуваний тип пакета:");
          if (CORE_DEBUG_LEVEL > 0)
            packet.printToLog();
          break;
      }
    }
    else if (_is_open && _clients.size() < _max_connection)
    {
      if (type == UdpPacket::TYPE_HANDSHAKE)
      {
        log_i("Приєднався клієнт з IP: %s", packet.getRemoteIP().toString());

        xSemaphoreTake(_client_mutex, portMAX_DELAY);
        _clients.try_emplace(packet.getRemoteIP(), packet.getRemoteIP(), packet.getRemotePort());
        xSemaphoreGive(_client_mutex);

        handleHandshake(packet);
      }
    }
  }

  void GameServer::packetHandlerTask(void* arg)
  {
    GameServer* self{static_cast<GameServer*>(arg)};
    UdpPacket* packet{nullptr};
    uint32_t processed_count{0};

    while (1)
    {
      if (xQueueReceive(self->_packet_queue, &packet, portMAX_DELAY) == pdPASS)
      {
        self->handlePacket(*packet);
        delete packet;

        if ((++processed_count & 31) == 0)
          delay(1);
      }
    }
  }

  void GameServer::onPacket(void* arg, AsyncUDPPacket& packet)
  {
    if (packet.length() > 1000 || packet.length() == 0)
    {
      log_e("Некоректний пакет. Size: %zu", packet.length());
      return;
    }

    GameServer* self{static_cast<GameServer*>(arg)};

    if (self->_packet_queue)
    {
      UdpPacket* pack = new UdpPacket(packet);

      if (xQueueSend(self->_packet_queue, &pack, 0) != pdPASS)
      {
        log_e("Черга _packet_queue переповнена");
        delete pack;
      }
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::pingClients()
  {
    xSemaphoreTake(_client_mutex, portMAX_DELAY);

    for (auto it = _clients.begin(), last_it = _clients.end(); it != last_it;)
    {
      if (!it->second.isConnected())
      {
        log_i("Клієнт від'єднався");

        if (it->second.isConfirmed())
        {
          --_confirmed_clients_num;
          invokeDisconnectHandler(it->second);
        }
        it = _clients.erase(it);
        last_it = _clients.end();
      }
      else
      {
        UdpPacket ping(1);
        ping.setType(UdpPacket::TYPE_PING);
        sendPacket(it->second, ping);
        ++it;
      }
    }

    xSemaphoreGive(_client_mutex);
  }

  void GameServer::pingClientsTask(void* arg)
  {
    GameServer* self = static_cast<GameServer*>(arg);

    while (1)
    {
      self->pingClients();
      delay(1000);
    }
  }

  void GameServer::handleNameConfirm(const String name, bool result)
  {
    _is_busy = false;

    ClientSession* client = findClient(name);
    if (!client)
      return;

    client->confirm();

    sendNameRespMsg(*client, result);

    if (!result)
      removeClient(client->getIP());
    else
      ++_confirmed_clients_num;
  }

  void GameServer::onConfirmationResult(const String name, bool result, GameServer* server)
  {
    server->handleNameConfirm(name, result);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::invokeConfirmationHandler(const ClientSession& client, ConfirmationResultHandler result_handler)
  {
    if (!_confirmation_handler) [[unlikely]]
    {
      log_e("Не додано обробник підключення клієнтів");
      sendNameRespMsg(client, false);
      _is_busy = false;
      return;
    }

    _confirmation_handler(client.getName(), result_handler, _confirmation_arg);
  }

  void GameServer::invokeDisconnectHandler(const ClientSession& client)
  {
    if (_disconnect_handler)
      _disconnect_handler(client.getName(), _disconnect_arg);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::onConfirmation(ClientConfirmationHandler handler, void* arg)
  {
    _confirmation_handler = handler;
    _confirmation_arg = arg;
  }

  void GameServer::onDisconnect(ClientDisconnectHandler handler, void* arg)
  {
    _disconnect_handler = handler;
    _disconnect_arg = arg;
  }

  void GameServer::onData(ClientDataHandler handler, void* arg)
  {
    _data_handler = handler;
    _data_arg = arg;
  }

  const std::unordered_map<uint32_t, ClientSession>* GameServer::getClients() const
  {
    return &_clients;
  }

  const char* GameServer::getServerIP() const
  {
    return _server_ip.c_str();
  }

  const char* GameServer::getName() const
  {
    return _server_name.c_str();
  }
}  // namespace pixeler
