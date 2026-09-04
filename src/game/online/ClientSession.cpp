#pragma GCC optimize("O3")
#include "ClientSession.h"

namespace pixeler
{
  ClientSession::ClientSession(IPAddress remote_ip, uint16_t port) : _remote_IP{remote_ip}, _port{port}
  {
    _last_act_time = millis();
  }

  IPAddress ClientSession::getIP() const
  {
    return _remote_IP;
  }

  uint16_t ClientSession::getPort() const
  {
    return _port;
  }

  void ClientSession::confirm()
  {
    _is_confirmed = true;
  }

  bool ClientSession::isConfirmed() const
  {
    return _is_confirmed;
  }

  void ClientSession::prolong()
  {
    _last_act_time = millis();
  }

  bool ClientSession::isConnected() const
  {
    return millis() - _last_act_time < 3000;
  }

  void ClientSession::setName(const String& name)
  {
    if (name.isEmpty()) [[unlikely]]
    {
      log_e("Ім'я клієнта не може бути порожнім");
      return;
    }

    _name = name;
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
