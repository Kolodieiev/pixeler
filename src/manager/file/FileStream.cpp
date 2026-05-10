#pragma GCC optimize("O3")

#include "FileStream.h"

#include "manager/FileManager.h"

namespace pixeler
{
  FileStream::FileStream(FILE* file, const char* name, size_t size)
  {
    open(file, name, size);
  }

  bool FileStream::open(FILE* file, const char* name, size_t size)
  {
    close();

    if (!file)
      return false;

    _file = file;
    _name = name;
    _size = size;

    if (!_cache)
    {
      _cache = static_cast<uint8_t*>(heap_caps_malloc(MAX_CACHE_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));

      if (!_cache)
      {
        log_e("Помилка виділення [ %zu ] байт для файлового потоку [ %s ]", MAX_CACHE_SIZE, name);
        _file = nullptr;
        _name = emptyString;
        return false;
      }
    }

    return true;
  }

  FileStream::~FileStream()
  {
    close();
    free(_cache);
    _cache = nullptr;
  }

  int FileStream::available()
  {
    if (!_file)
      return 0;

    return _size - _consumed_total;
  }

  bool FileStream::fillCache()
  {
    size_t remaining_in_file = _size - _consumed_total;
    if (remaining_in_file == 0)
      return false;

    size_t to_read = (remaining_in_file > MAX_CACHE_SIZE) ? MAX_CACHE_SIZE : remaining_in_file;

    size_t actually_read = _fs.readFromFile(_file, _cache, to_read);

    if (actually_read > 0)
    {
      _cache_pos = 0;
      _cache_available = actually_read;
      return true;
    }
    return false;
  }

  size_t FileStream::readBytes(char* out_buffer, size_t length)
  {
    if (_consumed_total >= _size)
      return 0;

    size_t total_written = 0;

    while (length > 0)
    {
      if (_cache_pos >= _cache_available)
      {
        if (!fillCache())
          break;
      }

      size_t can_copy = _cache_available - _cache_pos;
      size_t to_copy = (length > can_copy) ? can_copy : length;

      memcpy(out_buffer + total_written, _cache + _cache_pos, to_copy);

      _cache_pos += to_copy;
      _consumed_total += to_copy;
      total_written += to_copy;
      length -= to_copy;

      if (_consumed_total >= _size)
        break;
    }

    return total_written;
  }

  int FileStream::read()
  {
    return -1;  // stub
  }

  size_t FileStream::write(uint8_t byte)
  {
    return 0;  // stub
  }

  int FileStream::peek()
  {
    return -1;  // stub
  }

  void FileStream::close()
  {
    _fs.closeFile(_file);

    _consumed_total = 0;
    _cache_pos = 0;
    _cache_available = 0;
  }

  bool FileStream::seek(int32_t position)
  {
    if (!_file)
      return false;

    bool success = _fs.seekPos(_file, position, SEEK_SET);

    if (success)
    {
      _cache_pos = 0;
      _cache_available = 0;
      _consumed_total = position;
    }

    return success;
  }

  size_t FileStream::position() const
  {
    return _consumed_total;
  }

  const char* FileStream::name() const
  {
    return _name.c_str();
  }

  size_t FileStream::size() const
  {
    return _size;
  }

  FileStream::operator bool() const
  {
    return _file != nullptr;
  }
}  // namespace pixeler
