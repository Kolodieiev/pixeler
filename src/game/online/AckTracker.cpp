#include "AckTracker.h"

#include <algorithm>

namespace pixeler
{
  static const uint8_t MIN_CLIENTS_NUM{1};
  static const uint8_t MAX_CLIENTS_NUM{10};

  AckTracker::AckTracker()
  {
  }

  AckTracker::~AckTracker()
  {
    delete[] _clients;
  }

  void AckTracker::init(uint8_t max_clients)
  {
    _max_client_num = std::clamp<uint8_t>(max_clients, MIN_CLIENTS_NUM, MAX_CLIENTS_NUM);
    delete[] _clients;
    _clients = new IPAddress[MAX_CLIENTS_NUM];
  }

  bool AckTracker::addClient(const IPAddress& ip)
  {
    if (_clients_num >= MAX_CLIENTS_NUM)
      return false;

    if (findClientIndex(ip) != -1)
      return true;

    _clients[_clients_num] = ip;
    _active_mask |= (1UL << _clients_num);
    _ack_mask |= (1UL << _clients_num);
    ++_clients_num;

    return true;
  }

  std::vector<IPAddress> AckTracker::getUnackedClients() const
  {
    std::vector<IPAddress> unacked;
    unacked.reserve(_clients_num);

    for (uint8_t i = 0; i < _clients_num; ++i)
    {
      if ((_ack_mask & (1UL << i)) == 0)
        unacked.push_back(_clients[i]);
    }

    return unacked;
  }

  void AckTracker::removeClient(const IPAddress& ip)
  {
    int index = findClientIndex(ip);
    if (index == -1)
      return;

    uint8_t last = _clients_num - 1;

    if (index != last)
    {
      // Переносимо IP
      _clients[index] = _clients[last];

      // Зчитуємо ACK-біт останнього елемента (0 або 1)
      uint32_t last_acked = (_ack_mask >> last) & 1UL;

      // Скидаємо біт на позиції index і записуємо туди стан last_acked
      _ack_mask &= ~(1UL << index);
      _ack_mask |= (last_acked << index);
    }

    // Очищаємо біт колишнього останнього елемента
    _ack_mask &= ~(1UL << last);

    --_clients_num;
    _active_mask = (1UL << _clients_num) - 1UL;
  }

  void AckTracker::resetAllAcks()
  {
    _ack_mask = 0;
  }

  void AckTracker::resetAck(const IPAddress& ip)
  {
    int index = findClientIndex(ip);
    if (index != -1)
      _ack_mask &= ~(1UL << index);
  }

  void AckTracker::setAck(const IPAddress& ip)
  {
    int index = findClientIndex(ip);
    if (index != -1)
      _ack_mask |= (1UL << index);
  }

  bool AckTracker::isAllAcked() const
  {
    if (_active_mask == 0)
      return true;

    return (_ack_mask & _active_mask) == _active_mask;
  }

  uint8_t AckTracker::getClientsCount() const
  {
    return _clients_num;
  }

  int AckTracker::findClientIndex(const IPAddress& ip) const
  {
    for (uint8_t i = 0; i < _clients_num; ++i)
    {
      if (_clients[i] == ip)
        return i;
    }

    return -1;
  }
}  // namespace pixeler
