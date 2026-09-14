#pragma GCC optimize("O3")
#include "GameServer.h"

#include <algorithm>

#include "manager/WiFiManager.h"
#include "util/MutexGuard.h"

namespace pixeler
{
  static const uint8_t MAX_RESEND_COUNT{20};
  static const uint8_t MAX_CLIENT_NAME_LEN{40};

#ifndef GAME_SERVER_PORT
#define GAME_SERVER_PORT 777
#endif  // #ifndef GAME_SERVER_PORT

#ifndef SERVER_PACKS_QUEUE_SIZE
#define SERVER_PACKS_QUEUE_SIZE 32
#endif  // #ifndef SERVER_PACKS_QUEUE_SIZE

  GameServer::GameServer()
  {
    // Виправлення помилки assert failed: tcpip_api_call (Invalid mbox)
    _wifi_was_enabled = _wifi.isEnabled();

    if (!_wifi_was_enabled)
      _wifi.enable();
  }

  GameServer::~GameServer()
  {
    stop();

    if (!_wifi_was_enabled)
      _wifi.disable();
    else
      _wifi.enable();
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  bool GameServer::begin(const String& game_ID, const String& server_name, const String& pwd, uint8_t max_connection, bool is_local, uint8_t wifi_chan)
  {
    _game_id = game_ID,
    _max_clients = std::clamp<uint8_t>(max_connection, 1, MAX_WIFI_CONNECTION);
    _ack_tracker.init(_max_clients);
    _last_packet_id = 0;

    if (is_local)
    {
      String serv_name{server_name};
      String password{pwd};
      if (!_wifi.createAP(serv_name, password, MAX_WIFI_CONNECTION, wifi_chan))
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

    xTaskCreatePinnedToCore(pingClientsTask, "pingCl", (1024 / 2) * 7, this, 10, &_ping_task_handler, 1);
    xTaskCreatePinnedToCore(packetHandlerTask, "packHndl", (1024 / 2) * 20, this, 10, &_packet_task_handler, 1);

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

    if (_sessions_mutex)
      xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    _server.onPacket([](AsyncUDPPacket& packet) {});
    _server.close();
    _sessions.clear();

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
      xSemaphoreGive(_sessions_mutex);
      vSemaphoreDelete(_sessions_mutex);
      _sessions_mutex = nullptr;
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

    MutexGuard sess_lock(_sessions_mutex);

    _is_open = false;

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it;)
    {
      if (!it->second.isAccepted())
      {
        it = _sessions.erase(it);
        last_it = _sessions.end();
      }
      else
      {
        ++it;
      }
    }

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
    return _max_clients == _accepted_sessions_num;
  }

  void GameServer::broadcastStartGame()
  {
    close();
    broadcast(UdpPacket::TYPE_SERVICE_DATA, UdpPacket::SUBTYPE_START_GAME);
    log_i("Повідомлення про ПОЧАТОК гри розіслано");
  }

  void GameServer::broadcastStopGame()
  {
    broadcast(UdpPacket::TYPE_SERVICE_DATA, UdpPacket::SUBTYPE_STOP_GAME);
    log_i("Повідомлення про ЗАВЕРШЕННЯ гри розіслано");
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  bool GameServer::broadcast(UdpPacket::PacketType type, uint8_t subtype, size_t data_size, const void* data)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.setSubtype(subtype);
    pack.write(data, data_size);

    return broadcast(pack);
  }

  bool GameServer::broadcast(const UdpPacket& packet)
  {
    MutexGuard sess_lock(_sessions_mutex);

    if (!_ack_tracker.isAllAcked())
      return false;

    _last_packet = std::move(packet);
    _last_packet.setID(++_last_packet_id);

    _ack_tracker.resetAllAcks();

    MutexGuard udp_lock(_udp_mutex);
    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
    {
      if (it->second.isAccepted())
        _server.writeTo(_last_packet.raw(), _last_packet.length(), it->second.getIP(), it->second.getPort());
    }

    return true;
  }

  bool GameServer::send(const ClientSession& session, UdpPacket::PacketType type, uint8_t subtype, size_t data_size, const void* data)
  {
    UdpPacket pack(data_size);
    pack.setType(type);
    pack.setSubtype(subtype);
    pack.write(data, data_size);

    return send(session, pack);
  }

  bool GameServer::send(const IPAddress& remote_ip, const UdpPacket& packet)
  {
    ClientSession* session = findSession(remote_ip);
    if (session)
      return send(*session, packet);

    return false;
  }

  bool GameServer::send(const IPAddress& remote_ip, UdpPacket::PacketType type, uint8_t subtype, size_t data_size, const void* data)
  {
    ClientSession* session = findSession(remote_ip);
    if (session)
      return send(*session, type, subtype, data_size, data);

    return false;
  }

  bool GameServer::send(const ClientSession& session, const UdpPacket& packet)
  {
    if (!session.isAccepted())
      return false;

    MutexGuard udp_lock(_udp_mutex);

    if (!_ack_tracker.isAllAcked())
      return false;

    _last_packet = std::move(packet);
    _last_packet.setID(++_last_packet_id);

    _ack_tracker.resetAck(session.getIP());
    _server.writeTo(_last_packet.raw(), _last_packet.length(), session.getIP(), session.getPort());

    return true;
  }

  void GameServer::requestAcks()
  {
    if (_ack_tracker.isAllAcked())
      return;

    MutexGuard sess_lock(_sessions_mutex);

    std::vector<IPAddress> clients = _ack_tracker.getUnackedClients();

    if (_resend_counter < MAX_RESEND_COUNT)
    {
      ++_resend_counter;

      MutexGuard udp_lock(_udp_mutex);

      for (const auto& client_ip : clients)
        sendUnackedUnsafe(_last_packet, client_ip);
    }
    else
    {
      for (const auto& client_ip : clients)
        removeSessionUnsafe(client_ip);

      _resend_counter = 0;
    }
  }

  bool GameServer::isAllAcked() const
  {
    return _ack_tracker.isAllAcked();
  }

  void GameServer::sendUnacked(const UdpPacket& packet, const IPAddress& remote_ip)
  {
    ClientSession* session = findSession(remote_ip);
    if (session)
      sendUnacked(packet, remote_ip, session->getPort());
  }

  void GameServer::sendUnacked(const UdpPacket& packet, const IPAddress& remote_ip, uint16_t port)
  {
    xSemaphoreTake(_udp_mutex, portMAX_DELAY);
    sendUnackedUnsafe(packet, remote_ip, port);
    xSemaphoreGive(_udp_mutex);
  }

  void GameServer::sendUnackedUnsafe(const UdpPacket& packet, const IPAddress& remote_ip)
  {
    ClientSession* session = findSessionUnsafe(remote_ip);
    if (session)
      sendUnackedUnsafe(packet, remote_ip, session->getPort());
  }

  void GameServer::sendUnackedUnsafe(const UdpPacket& packet, const IPAddress& remote_ip, uint16_t port)
  {
    _server.writeTo(packet.raw(), packet.length(), remote_ip, port);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  IPAddress GameServer::getClientIP(const String& client_name)
  {
    MutexGuard sess_lock(_sessions_mutex);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
    {
      if (it->second.isAccepted() && it->second.hasName(client_name))
      {
        IPAddress ip = it->second.getIP();
        return ip;
      }
    }

    return IPAddress();
  }

  void GameServer::removeSession(const String& client_name)
  {
    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
    {
      if (it->second.isAccepted() && it->second.hasName(client_name))
      {
        const IPAddress remote_ip = it->second.getIP();
        _ack_tracker.removeClient(remote_ip);
        _sessions.erase(it);
        --_accepted_sessions_num;
        xSemaphoreGive(_sessions_mutex);
        invokeDisconnectHandler(remote_ip);
        break;
      }
    }
  }

  void GameServer::removeSession(const IPAddress& remote_ip)
  {
    MutexGuard sess_lock(_sessions_mutex);
    removeSessionUnsafe(remote_ip);
  }

  void GameServer::removeSessionUnsafe(const IPAddress& remote_ip)
  {
    bool is_accepted = false;

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    auto it = _sessions.find(remote_ip);
    if (it == _sessions.end())
    {
      xSemaphoreGive(_sessions_mutex);
      return;
    }

    if (it->second.isAccepted())
    {
      _ack_tracker.removeClient(it->second.getIP());
      --_accepted_sessions_num;
      is_accepted = true;
    }

    _sessions.erase(it);
    xSemaphoreGive(_sessions_mutex);

    if (is_accepted)
      invokeDisconnectHandler(remote_ip);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  ClientSession* GameServer::findSessionUnsafe(const IPAddress& remote_ip)
  {
    auto it = _sessions.find(remote_ip);
    if (it != _sessions.end())
      return &it->second;

    return nullptr;
  }

  ClientSession* GameServer::findSession(const String& name)
  {
    MutexGuard sess_lock(_sessions_mutex);

    for (auto it = _sessions.begin(), last_it = _sessions.end(); it != last_it; ++it)
      if (it->second.hasName(name))
        return &it->second;

    return nullptr;
  }

  ClientSession* GameServer::findSession(const IPAddress& remote_ip)
  {
    MutexGuard sess_lock(_sessions_mutex);
    return findSessionUnsafe(remote_ip);
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

  void GameServer::sendBusy(const UdpPacket& packet)
  {
    log_i("Сервер зайнятий");

    UdpPacket resp_packet;
    resp_packet.setType(UdpPacket::TYPE_CONNECT);
    resp_packet.setSubtype(UdpPacket::SUBTYPE_BUSY);

    sendUnacked(resp_packet, packet.getIP(), packet.getPort());
  }

  bool GameServer::openSession(const UdpPacket& packet)
  {
    MutexGuard sess_lock(_sessions_mutex);

    if (_is_open && _accepted_sessions_num < _max_clients)
    {
      _sessions.try_emplace(packet.getIP(), packet.getIP(), packet.getPort());
      return true;  // Не важливо, якщо не вставили через наявність ключа
    }
    else
    {
      log_e("Клієнт відхилений через закриття сервера");
      return false;
    }
  }

  void GameServer::handleHandshake(const UdpPacket& packet)
  {
    if (packet.getSubtype() != UdpPacket::SUBTYPE_HANDSHAKE)
      return;

    if (!_is_open || _accepted_sessions_num >= _max_clients)
    {
      sendBusy(packet);
      log_i("Підключення відхилено через заповнений/закритий сервер");
      return;
    }

    UdpPacket resp_packet;
    resp_packet.setType(UdpPacket::TYPE_CONNECT);

    if (packet.isDataEquals(_game_id.c_str()))
    {
      if (openSession(packet))
      {
        resp_packet.setSubtype(UdpPacket::SUBTYPE_HANDSHAKE);
        log_i("Відкрито сеанс для IP: %s", packet.getIP().toString());
      }
      else
      {
        sendBusy(packet);
        return;
      }
    }
    else
    {
      resp_packet.setSubtype(UdpPacket::SUBTYPE_INCORRECT_SERVER);
      log_i("Невдала спроба приєднатися ігровим клієнтом [%s] до [%s]", packet.dataToString().c_str(), _game_id.c_str());
    }

    sendUnacked(resp_packet, packet.getIP(), packet.getPort());
  }

  void GameServer::sendNameRespMsg(const ClientSession& session, bool result)
  {
    UdpPacket resp_packet;
    resp_packet.setType(UdpPacket::TYPE_CONNECT);

    if (!result)
    {
      resp_packet.setSubtype(UdpPacket::SUBTYPE_ACCESS_DENIED);
      log_i("Авторизацію відхилено: %s", session.getName());
    }
    else
    {
      resp_packet.setSubtype(UdpPacket::SUBTYPE_ACCESS_GRANTED);
      log_i("Авторизовано: %s", session.getName());
    }

    sendUnacked(resp_packet, session.getIP(), session.getPort());
  }

  void GameServer::sendIncorrectName(const ClientSession& session)
  {
    log_i("Некоректне ім'я клієнта");

    UdpPacket resp_packet;
    resp_packet.setType(UdpPacket::TYPE_CONNECT);
    resp_packet.setSubtype(UdpPacket::SUBTYPE_INCORRECT_NAME);

    sendUnacked(resp_packet, session.getIP(), session.getPort());
  }

  void GameServer::handleLogin(ClientSession& session, const UdpPacket& packet)
  {
    if (packet.getSubtype() != UdpPacket::SUBTYPE_LOGIN)
      return;

    if (_is_busy)
    {
      sendBusy(packet);
      removeSession(session.getIP());
      return;
    }

    log_i("Запит авторизації");

    if (session.isAccepted())
    {
      /* Помилкова повторна авторизація вже авторизованого клієнта.
       * Мусить дочекатися видалення з сервера через 3 сек,
       * якщо відключився випадково самостійно. */
      sendNameRespMsg(session, false);
      return;
    }

    String client_name = packet.dataToString();
    if (client_name.isEmpty() ||
        client_name.length() > MAX_CLIENT_NAME_LEN ||
        client_name.equals(_server_name) ||
        findSession(client_name))
    {
      sendIncorrectName(session);
      removeSession(session.getIP());
      return;
    }

    /* Якщо сервер вже приймає рішення щодо авторизації клієнта,
     * інші клієнти не повинні переривати розгляд поточного рішення. */
    _is_busy = true;

    session.setName(client_name);
    invokeAcceptHandler(session, client_name);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::handlePacket(const UdpPacket& packet)
  {
    UdpPacket::PacketType type = packet.getType();
    ClientSession* session = findSession(packet.getIP());
    if (!session) [[unlikely]]
    {
      if (packet.getType() == UdpPacket::TYPE_CONNECT)
        handleHandshake(packet);
      else
        log_i("Проігноровано пакет без сесії");

      return;
    }

    session->prolong();

    switch (type)
    {
      case UdpPacket::TYPE_GAME_DATA:
        invokeDataHandler(*session, packet);
        break;

      case UdpPacket::TYPE_CONNECT:
        handleLogin(*session, packet);
        break;

      default:
        if (type != UdpPacket::TYPE_PING)
        {
          log_e("Неочікуваний пакет:");
#if CORE_DEBUG_LEVEL > 0
          packet.printToLog();
#endif  // CORE_DEBUG_LEVEL > 0
        }
        break;
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
    if (packet_len > MAX_PACKET_SIZE || packet_len < PACKET_EXTRA_SIZE) [[unlikely]]
    {
      log_e("Некоректний розмір пакета: %zu", packet_len);
      return;
    }

    GameServer& self = *static_cast<GameServer*>(arg);
    UdpPacket* pack = new UdpPacket(packet);

    if (!self.isAllAcked())
    {
      if (pack->getType() == UdpPacket::TYPE_SERVICE_DATA && pack->getSubtype() == UdpPacket::SUBTYPE_ACK)
        self.handleAck(*pack);

      delete pack;
      return;
    }

    if (xQueueSend(self._packet_queue, &pack, 0) != pdPASS) [[unlikely]]
    {
      log_e("Черга packet_queue переповнена");
      delete pack;
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::pingClients()
  {
    IPAddress disconnect_buf[_accepted_sessions_num];
    size_t disconnect_count = 0;

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);

    for (auto it = _sessions.begin(); it != _sessions.end();)
    {
      auto& session = it->second;

      if (!session.isConnected())
      {
        if (session.isAccepted())
        {
          _ack_tracker.removeClient(session.getIP());
          --_accepted_sessions_num;
          log_i("Клієнт від'єднався: %s", session.getName().c_str());

          disconnect_buf[disconnect_count++] = session.getIP();
        }

        it = _sessions.erase(it);
      }
      else
      {
        if (session.needsPing())
        {
          UdpPacket packet;
          packet.setType(UdpPacket::TYPE_PING);
          sendUnacked(packet, session.getIP(), session.getPort());
        }
        ++it;
      }
    }

    xSemaphoreGive(_sessions_mutex);

    for (size_t i = 0; i < disconnect_count; ++i)
      invokeDisconnectHandler(disconnect_buf[i]);
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
    _is_busy = false;

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
      session->accept();
      ++_accepted_sessions_num;
    }
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::invokeAcceptHandler(const ClientSession& session, const String& client_name)
  {
    if (!_accept_handler)
    {
      log_e("Не додано обробник підключення клієнтів");
      sendNameRespMsg(session, false);
      removeSession(session.getIP());
      _is_busy = false;
      return;
    }

    _accept_handler(client_name, _accept_arg);
  }

  void GameServer::invokeDisconnectHandler(const IPAddress& remote_ip)
  {
    if (!_disconnect_handler)
    {
      log_e("Не встановлено обробник відключення клієнтів");
      return;
    }

    _disconnect_handler(remote_ip, _disconnect_arg);
  }

  void GameServer::invokeDataHandler(const ClientSession& session, const UdpPacket& packet)
  {
    if (!session.isAccepted()) [[unlikely]]
    {
      log_e("Неавторизовані ігрові дані");
      removeSession(session.getIP());
      return;
    }

    if (!_data_handler) [[unlikely]]
    {
      log_e("Не встановлено обробник клієнтських даних");
      return;
    }

    _data_handler(session, packet, _data_arg);
  }

  void GameServer::handleAck(const UdpPacket& packet)
  {
    if (packet.getDataLen() < 1)
    {
      log_e("Відсутній ack-ID");
      removeSession(packet.getIP());
      return;
    }

    uint8_t id = packet.getID();
    if (id != _last_packet_id) [[unlikely]]
    {
      log_e("Некоректний ACK. Надіслано %u, повернуто %u", _last_packet_id, id);
      return;
    }

    xSemaphoreTake(_sessions_mutex, portMAX_DELAY);
    _ack_tracker.setAck(packet.getIP());
    xSemaphoreGive(_sessions_mutex);
  }

  // ------------------------------------------------------------------------------------------------------------------------------

  void GameServer::onAccept(AcceptHandler handler, void* arg)
  {
    _accept_handler = handler;
    _accept_arg = arg;
  }

  void GameServer::onDisconnect(DisconnectHandler handler, void* arg)
  {
    _disconnect_handler = handler;
    _disconnect_arg = arg;
  }

  void GameServer::onGameData(DataHandler handler, void* arg)
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
