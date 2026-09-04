#pragma GCC optimize("O3")
#include "UdpPacket.h"

#include <cstring>

namespace pixeler
{
  static const uint8_t PACKET_EXTRA_SIZE = 2;  // Основний тип пакета 1 байт + його підтип 1 байт
  static const uint16_t MAX_DATA_SIZE = 998;   // Максимальний розмір даних, які можуть бути записані до пакету

  UdpPacket::UdpPacket(AsyncUDPPacket& packet) : DataStream(packet.length() < PACKET_EXTRA_SIZE ? PACKET_EXTRA_SIZE : packet.length())
  {
    _data_length = _length - PACKET_EXTRA_SIZE;
    _index = PACKET_EXTRA_SIZE;

    memcpy(_buffer, packet.data(), packet.length());

    _remote_ip = packet.remoteIP();
    _port = packet.remotePort();
  }

  UdpPacket::UdpPacket() : UdpPacket(0) {}

  UdpPacket::UdpPacket(size_t data_len) : DataStream(data_len > MAX_DATA_SIZE ? MAX_DATA_SIZE + PACKET_EXTRA_SIZE : data_len + PACKET_EXTRA_SIZE)
  {
    if (data_len > MAX_DATA_SIZE) [[unlikely]]
    {
      log_e("Некоректний розмір пакета [%zu] обрізано до [%zu]", data_len, _length);
    }

    _buffer[0] = TYPE_DATA;

    _data_length = _length - PACKET_EXTRA_SIZE;
    _index = PACKET_EXTRA_SIZE;
  }

  void UdpPacket::setType(PacketType type)
  {
    _buffer[0] = type;
  }

  UdpPacket::PacketType UdpPacket::getType() const
  {
    return static_cast<PacketType>(_buffer[0]);
  }

  void UdpPacket::setSubtype(uint8_t subtype)
  {
    _buffer[1] = subtype;
  }

  uint8_t UdpPacket::getSubtype() const
  {
    return _buffer[1];
  }

  void UdpPacket::printToLog(bool char_like) const
  {
    log_i("PacketType: %d", _buffer[0]);
    log_i("PacketSubtype: %d", _buffer[1]);
    log_i("Data size: %zu", _data_length);
    log_i("Data:");

    if (char_like)
    {
      for (size_t i = PACKET_EXTRA_SIZE; i < _length; ++i)
        log_i("%c", _buffer[i]);
    }
    else
    {
      for (size_t i = PACKET_EXTRA_SIZE; i < _length; ++i)
        log_i("%#04x", _buffer[i]);
    }
  }

  IPAddress UdpPacket::getRemoteIP() const
  {
    return _remote_ip;
  }

  uint16_t UdpPacket::getRemotePort() const
  {
    return _port;
  }

  bool UdpPacket::isDataEquals(const void* data, size_t data_len, size_t start_pos) const
  {
    if (!data || start_pos >= _data_length)
      return false;

    if (data_len == 0)
      data_len = _data_length - start_pos;
    else if (start_pos + data_len > _data_length)
      return false;

    return std::memcmp(&_buffer[start_pos + PACKET_EXTRA_SIZE], data, data_len) == 0;
  }

  size_t UdpPacket::extractBytes(void* out, size_t start_pos, size_t len) const
  {
    if (start_pos >= _data_length)
      return 0;

    size_t available = _data_length - start_pos;

    if (len > available)
      len = available;

    memcpy(out, _buffer + start_pos + PACKET_EXTRA_SIZE, len);

    return len;
  }

  const uint8_t* UdpPacket::getData(uint16_t data_pos) const
  {
    if (data_pos >= _data_length)
    {
      log_e("Некоректна позиція. Розмір даних: %zu Позиція: %d ", _data_length, data_pos);
      data_pos = 0;
    }

    return _buffer + data_pos + PACKET_EXTRA_SIZE;
  }

  String UdpPacket::dataToString() const
  {
    char pack_data[_data_length + 1];
    pack_data[_data_length] = '\0';
    memcpy(pack_data, _buffer + PACKET_EXTRA_SIZE, _data_length);
    return String(pack_data);
  }

  size_t UdpPacket::getDataLen() const
  {
    return _data_length;
  }

  size_t UdpPacket::getDataIndex() const
  {
    return _index - PACKET_EXTRA_SIZE;
  }

  void UdpPacket::resetDataIndex()
  {
    _index = PACKET_EXTRA_SIZE;
  }
}  // namespace pixeler
