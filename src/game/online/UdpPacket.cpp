#pragma GCC optimize("O3")
#include "UdpPacket.h"

#include <cstring>

namespace pixeler
{
  static const uint16_t MAX_DATA_SIZE = MAX_PACKET_SIZE - PACKET_EXTRA_SIZE;  // Максимальний розмір даних, які можуть бути записані до пакета.

  static const int PACKET_TYPE_POS = 0;
  static const int PACKET_SUBTYPE_POS = 1;
  static const int PACKET_ID_FLAG_POS = 2;
  static const int PACKET_ID_POS = 3;

  UdpPacket::UdpPacket(const UdpPacket& other) : DataStream(other._length)
  {
    if (other._buffer && _buffer)
    {
      _length = other._length;
      _data_length = other._data_length;
      _index = other._index;
      _remote_ip = other._remote_ip;
      _port = other._port;

      memcpy(_buffer, other._buffer, _length);
    }
    else
    {
      _length = 0;
      _data_length = 0;
      _index = 0;
      _remote_ip = IPAddress();
      _port = 0;
    }
  }

  UdpPacket& UdpPacket::operator=(const UdpPacket& other)
  {
    if (this == &other)
      return *this;

    resize(other._length);

    if (other._buffer && _buffer && other._length > 0)
    {
      _length = other._length;
      _data_length = other._data_length;
      _index = other._index;
      _remote_ip = other._remote_ip;
      _port = other._port;

      memcpy(_buffer, other._buffer, _length);
    }
    else
    {
      _length = 0;
      _data_length = 0;
      _index = 0;
      _remote_ip = IPAddress();
      _port = 0;
    }

    return *this;
  }

  UdpPacket::UdpPacket(UdpPacket&& other)
  {
    _length = other._length;
    _data_length = other._data_length;
    _index = other._index;
    _buffer = other._buffer;
    _remote_ip = other._remote_ip;
    _port = other._port;

    other._length = 0;
    other._data_length = 0;
    other._index = 0;
    other._buffer = nullptr;
  }

  UdpPacket& UdpPacket::operator=(UdpPacket&& other)
  {
    if (this == &other)
      return *this;

    delete[] _buffer;

    _length = other._length;
    _data_length = other._data_length;
    _index = other._index;
    _buffer = other._buffer;
    _remote_ip = other._remote_ip;
    _port = other._port;

    other._length = 0;
    other._data_length = 0;
    other._index = 0;
    other._buffer = nullptr;

    return *this;
  }

  UdpPacket::UdpPacket(AsyncUDPPacket& packet) : DataStream(packet.length() < PACKET_EXTRA_SIZE ? PACKET_EXTRA_SIZE : packet.length())
  {
    _data_length = _length - PACKET_EXTRA_SIZE;
    _index = PACKET_EXTRA_SIZE;

    memcpy(_buffer, packet.data(), packet.length());

    _remote_ip = packet.remoteIP();
    _port = packet.remotePort();
  }

  UdpPacket::UdpPacket() : UdpPacket(0) {}

  UdpPacket::UdpPacket(size_t data_len) : DataStream(data_len > MAX_DATA_SIZE ? MAX_PACKET_SIZE : data_len + PACKET_EXTRA_SIZE)
  {
    _data_length = _length - PACKET_EXTRA_SIZE;
    _index = PACKET_EXTRA_SIZE;

    if (data_len > MAX_DATA_SIZE)
    {
      log_e("Некоректний розмір даних пакета [%zu]. Максимально можливий: [%zu]", data_len, MAX_DATA_SIZE);
      esp_restart();
    }

    _buffer[PACKET_TYPE_POS] = TYPE_GAME_DATA;
    _buffer[PACKET_ID_FLAG_POS] = 0;
    _buffer[PACKET_ID_POS] = 0;
  }

  void UdpPacket::setType(PacketType type)
  {
    _buffer[PACKET_TYPE_POS] = type;
  }

  UdpPacket::PacketType UdpPacket::getType() const
  {
    return static_cast<PacketType>(_buffer[PACKET_TYPE_POS]);
  }

  void UdpPacket::setSubtype(uint8_t subtype)
  {
    _buffer[PACKET_SUBTYPE_POS] = subtype;
  }

  uint8_t UdpPacket::getSubtype() const
  {
    return _buffer[PACKET_SUBTYPE_POS];
  }

  void UdpPacket::setID(uint8_t id)
  {
    _buffer[PACKET_ID_POS] = id;
    _buffer[PACKET_ID_FLAG_POS] = 1;
  }

  bool UdpPacket::hasID() const
  {
    return _buffer[PACKET_ID_FLAG_POS];
  }

  uint8_t UdpPacket::getID() const
  {
    return _buffer[PACKET_ID_POS];
  }

  void UdpPacket::printToLog(bool char_like) const
  {
    log_i("PacketType: %d", _buffer[PACKET_TYPE_POS]);
    log_i("PacketSubtype: %d", _buffer[PACKET_SUBTYPE_POS]);
    log_i("PacketID: %d", _buffer[PACKET_ID_POS]);
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

  IPAddress UdpPacket::getIP() const
  {
    return _remote_ip;
  }

  uint16_t UdpPacket::getPort() const
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
