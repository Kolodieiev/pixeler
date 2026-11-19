#pragma GCC optimize("O3")
#include "DataStream.h"

namespace pixeler
{
  DataStream::DataStream(size_t size)
  {
    resize(size);
  }

  DataStream::~DataStream()
  {
    delete[] _buffer;
  }

  void DataStream::resize(size_t size)
  {
    _index = 0;

    if (size == _length && (_buffer != nullptr || size == 0))
      return;

    delete[] _buffer;
    _buffer = nullptr;
    _length = size;

    if (size == 0)
      return;

    try
    {
      _buffer = new uint8_t[size];
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  size_t DataStream::read(void* out, size_t len)
  {
    if (len == 0)
      return 0;

    const size_t s = space();
    if (len > s)
      len = s;

    memcpy(out, _buffer + _index, len);
    _index += len;
    return len;
  }

  size_t DataStream::drop(size_t len)
  {
    if (len == 0)
      return 0;

    const size_t s = space();
    if (len > s)
      len = s;

    _index += len;

    return len;
  }

  size_t DataStream::write(const void* data, size_t len)
  {
    if (len < 1)
      return 0;

    size_t s = space();
    if (len > s)
      len = s;

    memcpy(_buffer + _index, data, len);
    _index += len;
    return len;
  }

  size_t DataStream::space() const
  {
    return _length - _index;
  }

  size_t DataStream::length() const
  {
    return _length;
  }

  size_t DataStream::index() const
  {
    return _index;
  }

  void DataStream::flush()
  {
    _index = 0;
  }

  const uint8_t* DataStream::raw() const
  {
    return _buffer;
  }

  DataStream::operator bool() const
  {
    return _buffer != nullptr;
  }
}  // namespace pixeler
