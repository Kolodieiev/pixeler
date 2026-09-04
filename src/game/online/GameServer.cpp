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

  bool GameServer::begin(const String& game_ID, const String& server_name, const String& pwd, uint8_t max_connection, bool is_local, uint8_t wifi_chan)
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

    _sessions_mutex = xSemaphoreCreateMutex();
    _udp_mutex = xSemaphoreCreateMutex();

    if (!_sessions_mutex)
    {
      log_e("Не вдалося створити _sessions_mutex");
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

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    _server.close();
    _sessions.clear();

    xSemaphoreGive(_sessions_mutex);

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

    if (_sessions_mutex)
    {
      vSemaphoreDelete(_sessions_mutex);
      _sessions_mutex = nullptr;
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
    if (!_is_open)
      return;

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);
    _is_open = false;

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it;)
    {
      if (!it->second.isConfirmed())
      {
        it = _sessions.erase(it);
        last_it = _sessions.end();
      }
      else
      {
        ++it;
      }
    }
    xSemaphoreGive(_sessions_mutex);
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
    return _max_connection == _confirmed_sessions_num;
  }

  void GameServer::broadcastGameStarted()
  {
    close();

    UdpPacket packet;
    packet.setType(UdpPacket::TYPE_CLIENT_DATA);
    packet.setSubtype(UdpPacket::SUBTYPE_START_GAME);

    sendBroadcast(packet);

    log_i("Повідомлення про СТАРТ розіслано");
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::sendBroadcast(UdpPacket::PacketType type, uint8_t subtype, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.setSubtype(subtype);
    pack.write(data, data_size);
    sendBroadcast(pack);
  }

  void GameServer::sendBroadcast(const UdpPacket& packet)
  {
    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
      sendPacket(it->second, packet);

    xSemaphoreGive(_sessions_mutex);
  }

  void GameServer::sendPacket(const ClientSession& session, UdpPacket::PacketType type, uint8_t subtype, const void* data, size_t data_size)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.setSubtype(subtype);
    pack.write(data, data_size);

    sendPacket(session, pack);
  }

  void GameServer::sendPacket(const ClientSession& session, const UdpPacket& packet)
  {
    xSemaphoreTake(_udp_mutex, portMAX_DELAY);
    _server.writeTo(packet.raw(), packet.length(), session.getIP(), session.getPort());
    xSemaphoreGive(_udp_mutex);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::removeSession(const String& client_name)
  {
    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
    {
      if (it->second.hasName(client_name))
      {
        _sessions.erase(it);
        break;
      }
    }

    xSemaphoreGive(_sessions_mutex);
  }

  void GameServer::removeSession(const IPAddress& remote_ip)
  {
    uint32_t session_ip = remote_ip;
    if (session_ip == 0)
      return;

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    auto it = _sessions.find(session_ip);
    if (it != _sessions.end())
      _sessions.erase(it);

    xSemaphoreGive(_sessions_mutex);
  }

  ClientSession* GameServer::findSession(const IPAddress& remote_ip)
  {
    uint32_t ip = remote_ip;
    if (ip == 0U)
      return nullptr;

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    auto it = _sessions.find(remote_ip);

    if (it == _sessions.end())
    {
      xSemaphoreGive(_sessions_mutex);
      return nullptr;
    }

    xSemaphoreGive(_sessions_mutex);
    return &it->second;
  }

  ClientSession* GameServer::findSession(const String& name)
  {
    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
      if (it->second.hasName(name))
      {
        xSemaphoreGive(_sessions_mutex);
        return &it->second;
      }

    xSemaphoreGive(_sessions_mutex);
    return nullptr;
  }

  const ClientSession* GameServer::findSession(const IPAddress& remote_ip) const
  {
    return findSession(remote_ip);
  }

  const ClientSession* GameServer::findSession(const String& name) const
  {
    return findSession(name);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::sendNameRespMsg(const ClientSession& session, bool result)
  {
    UdpPacket packet;
    packet.setType(UdpPacket::TYPE_CONNECT);

    if (!result)
    {
      packet.setSubtype(UdpPacket::SUBTYPE_ACCESS_DENIED);
      log_i("Авторизацію відхилено: %s", session.getName());
    }
    else
    {
      packet.setSubtype(UdpPacket::SUBTYPE_ACCESS_GRANTED);
      log_i("Авторизовано: %s", session.getName());
    }

    sendPacket(session, packet);
  }

  void GameServer::sendIncorrectName(const ClientSession& session)
  {
    log_i("Некоректне ім'я клієнта");

    UdpPacket packet;
    packet.setType(UdpPacket::TYPE_CONNECT);
    packet.setSubtype(UdpPacket::SUBTYPE_INCORRECT_NAME);

    sendPacket(session, packet);
  }

  void GameServer::sendBusy(const ClientSession& session)
  {
    log_i("Сервер зайнятий");

    UdpPacket packet;
    packet.setType(UdpPacket::TYPE_CONNECT);
    packet.setSubtype(UdpPacket::SUBTYPE_BUSY);

    sendPacket(session, packet);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::handleHandshake(const UdpPacket& packet)
  {
    UdpPacket out_packet;
    out_packet.setType(UdpPacket::TYPE_CONNECT);

    if (packet.isDataEquals(_game_id.c_str()))
    {
      out_packet.setSubtype(UdpPacket::SUBTYPE_HANDSHAKE);
      log_i("Відкрито сеанс для IP: %s", packet.getRemoteIP().toString());
    }
    else
    {
      out_packet.setSubtype(UdpPacket::SUBTYPE_INCORRECT_SERVER);
      log_i("Невдала спроба приєднатися ігровим клієнтом [%s] до [%s]", packet.dataToString().c_str(), _game_id.c_str());
    }

    _server.writeTo(out_packet.raw(), out_packet.length(), packet.getRemoteIP(), packet.getRemotePort());
  }

  void GameServer::handleLogin(ClientSession& session, const UdpPacket& packet)
  {
    if (packet.getSubtype() != UdpPacket::SUBTYPE_LOGIN)
      return;

    log_i("Запит авторизації");

    if (session.isConfirmed())
    {
      /* Помилкова повторна авторизація вже авторизованого клієнта.
       * Мусить дочекатися видалення з сервера через 3 сек,
       * якщо відключився випадково самостійно. */
      sendNameRespMsg(session, false);
      return;
    }

    String client_name = packet.dataToString();
    if (packet.getDataLen() > 40 || client_name.equals(_server_name) || findSession(client_name))
    {
      sendIncorrectName(session);
      removeSession(session.getIP());
      return;
    }

    if (_is_busy)
    {
      sendBusy(session);
      removeSession(session.getIP());
      return;
    }

    /* Якщо сервер вже приймає рішення щодо авторизації клієнта,
     * інші клієнти не повинні переривати розгляд поточного рішення. */
    _is_busy = true;

    session.setName(client_name);
    invokeConfirmationHandler(session, client_name);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::handlePacket(const UdpPacket& packet)
  {
    UdpPacket::PacketType type = packet.getType();
    ClientSession* session = findSession(packet.getRemoteIP());

    if (session)
    {
      switch (type)
      {
        case UdpPacket::TYPE_DATA:
          invokeDataHandler(*session, packet);
          break;
        case UdpPacket::TYPE_PING:
          session->prolong();
          break;
        case UdpPacket::TYPE_CONNECT:
          handleLogin(*session, packet);
          break;
        default:
          log_e("Неочікуваний пакет:");
          if (CORE_DEBUG_LEVEL > 0)
            packet.printToLog();
          break;
      }
    }
    else
    {
      tryAddSession(packet);
    }
  }

  void GameServer::tryAddSession(const UdpPacket& packet)
  {
    if (_is_open && _sessions.size() < _max_connection)
    {
      UdpPacket::PacketType type = packet.getType();
      uint8_t subtype = packet.getSubtype();
      if (type == UdpPacket::TYPE_CONNECT && subtype == UdpPacket::SUBTYPE_HANDSHAKE)
      {
        xSemaphoreTake(_sessions_mutex, portMAX_DELAY);
        if (_is_open && _sessions.size() < _max_connection)
        {
          _sessions.try_emplace(packet.getRemoteIP(), packet.getRemoteIP(), packet.getRemotePort());
          xSemaphoreGive(_sessions_mutex);
          handleHandshake(packet);
        }
        else
        {
          xSemaphoreGive(_sessions_mutex);
          log_e("Клієнт відхилений через закриття сервера");
        }
      }
      else
      {
        log_e("Неавторизований пакет:");
        if (CORE_DEBUG_LEVEL > 0)
          packet.printToLog();
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
    size_t packet_len = packet.length();
    if (packet_len > 1000 || packet_len < 2)
    {
      log_e("Некоректний розмір пакета: %zu", packet_len);
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
    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it;)
    {
      if (!it->second.isConnected())
      {
        log_i("Клієнт від'єднався: %s", it->second.getName().c_str());

        if (it->second.isConfirmed())
        {
          --_confirmed_sessions_num;
          invokeDisconnectHandler(it->second);
        }
        it = _sessions.erase(it);
        last_it = _sessions.end();
      }
      else
      {
        UdpPacket ping(1);
        ping.setType(UdpPacket::TYPE_PING);
        sendPacket(it->second, ping);
        ++it;
      }
    }

    xSemaphoreGive(_sessions_mutex);
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

  void GameServer::resolveJoin(const String& client_name, bool is_accepted)
  {
    ClientSession* session = findSession(client_name);
    if (!session)
      return;

    sendNameRespMsg(*session, is_accepted);

    if (!is_accepted)
    {
      removeSession(session->getIP());
    }
    else
    {
      session->confirm();
      ++_confirmed_sessions_num;
    }

    _is_busy = false;
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::invokeConfirmationHandler(const ClientSession& session, const String& client_name)
  {
    if (!_confirmation_handler) [[unlikely]]
    {
      log_e("Не додано обробник підключення клієнтів");
      sendNameRespMsg(session, false);
      removeSession(session.getIP());
      _is_busy = false;
      return;
    }

    _confirmation_handler(client_name, _confirmation_arg);
  }

  void GameServer::invokeDisconnectHandler(const ClientSession& session)
  {
    if (!_disconnect_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник відключення клієнтів");
      return;
    }

    _disconnect_handler(session.getName(), _disconnect_arg);
  }

  void GameServer::invokeDataHandler(const ClientSession& session, const UdpPacket& packet)
  {
    if (!session.isConfirmed()) [[unlikely]]
    {
      removeSession(session.getIP());
      return;
    }

    if (!_data_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник даних від клієнтів");
      return;
    }

    _data_handler(session, packet, _data_arg);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::onConfirmation(ConfirmationHandler handler, void* arg)
  {
    _confirmation_handler = handler;
    _confirmation_arg = arg;
  }

  void GameServer::onDisconnect(DisconnectHandler handler, void* arg)
  {
    _disconnect_handler = handler;
    _disconnect_arg = arg;
  }

  void GameServer::onData(DataHandler handler, void* arg)
  {
    _data_handler = handler;
    _data_arg = arg;
  }

  const std::unordered_map<uint32_t, ClientSession>* GameServer::getClients() const
  {
    return &_sessions;
  }

  String GameServer::getServerIP() const
  {
    return _server_ip;
  }

  String GameServer::getName() const
  {
    return _server_name;
  }
}  // namespace pixeler
