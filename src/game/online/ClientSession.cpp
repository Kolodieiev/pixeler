#pragma GCC optimize("O3")
#include "ClientSession.h"

namespace pixeler
{
  static const uint16_t MAX_PING_TIME = 4000U;
  static const uint16_t PING_INTERVAL = 2000U;

  ClientSession::ClientSession(IPAddress remote_ip, uint16_t port) : _remote_IP{remote_ip}, _port{port}
  {
    _last_act_time = millis();
  }

  ClientSession::ClientSession(ClientSession&& other) noexcept
      : _remote_IP(other._remote_IP),
        _name(std::move(other._name)),
        _last_act_time(other._last_act_time),
        _port(other._port)
  {
  }

  ClientSession& ClientSession::operator=(ClientSession&& other) noexcept
  {
    if (this == &other)
      return *this;

    _remote_IP = other._remote_IP;
    _name = std::move(other._name);
    _last_act_time = other._last_act_time;
    _port = other._port;

    return *this;
  }

  IPAddress ClientSession::getIP() const
  {
    return _remote_IP;
  }

  uint16_t ClientSession::getPort() const
  {
    return _port;
  }

  void ClientSession::accept()
  {
    _is_accepted = true;
  }

  bool ClientSession::isAccepted() const
  {
    return _is_accepted;
  }

  void ClientSession::prolong()
  {
    _last_act_time = millis();
  }

  bool ClientSession::isConnected() const
  {
    return millis() - _last_act_time < MAX_PING_TIME;
  }

  void ClientSession::setName(const String& name)
  {
    if (name.isEmpty())
    {
      log_e("Ім'я клієнта не може бути порожнім");
      return;
    }

    _name = name;
  }

  bool ClientSession::needsPing() const
  {
    return millis() - _last_act_time > PING_INTERVAL;
  }

  String ClientSession::getName() const
  {
    return _name;
  }

  bool ClientSession::hasName(const String& name) const
  {
    return _name.equals(name);
  }

  bool ClientSession::is(const ClientSession& session) const
  {
    return _remote_IP == session._remote_IP;
  }

  bool ClientSession::is(const IPAddress& remote_ip) const
  {
    return _remote_IP == remote_ip;
  }
}  // namespace pixeler
