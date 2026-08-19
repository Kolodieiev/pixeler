#pragma GCC optimize("O3")
#include "ClientWrapper.h"

namespace pixeler
{
  ClientWrapper::ClientWrapper(IPAddress remote_ip, uint16_t port) : _remote_ip{remote_ip}, _port{port}
  {
    _last_act_time = millis();
  }

  IPAddress ClientWrapper::getIP() const
  {
    return _remote_ip;
  }

  uint16_t ClientWrapper::getPort() const
  {
    return _port;
  }

  void ClientWrapper::confirm()
  {
    _is_confirmed = true;
  }

  bool ClientWrapper::isConfirmed() const
  {
    return _is_confirmed;
  }

  void ClientWrapper::prolong()
  {
    _last_act_time = millis();
  }

  bool ClientWrapper::isConnected() const
  {
    return millis() - _last_act_time < 3000;
  }

  void ClientWrapper::setName(const char* name)
  {
    if (name)
      _name = name;
    else
      log_e("Ім'я клієнта не може бути null");
  }

  const char* ClientWrapper::getName() const
  {
    return _name.c_str();
  }

  bool ClientWrapper::hasName(const char* name) const
  {
    if (!name)
      return false;

    return strcmp(_name.c_str(), name) == 0;
  }

  bool ClientWrapper::is(const ClientWrapper* client_wrap) const
  {
    if (!client_wrap)
      return false;

    return _remote_ip == client_wrap->_remote_ip;
  }
}  // namespace pixeler
