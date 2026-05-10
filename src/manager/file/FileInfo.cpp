#pragma GCC optimize("O3")

#include "FileInfo.h"

#include <Arduino.h>

#include <cctype>
#include <cstring>

namespace pixeler
{
  FileInfo::FileInfo(const String& name, bool is_dir) : _is_dir{is_dir}, _name_len{name.length()}
  {
    if (psramInit())
      _name = static_cast<char*>(ps_malloc(_name_len + 1));
    else
      _name = static_cast<char*>(malloc(_name_len + 1));

    if (!_name)
    {
      log_e("Помилка виділення буферу");
      esp_restart();
    }

    std::memcpy(_name, name.c_str(), _name_len);
    _name[_name_len] = '\0';
  }

  FileInfo::~FileInfo()
  {
    free(_name);
  }

  bool FileInfo::isDir() const
  {
    return _is_dir;
  }

  bool FileInfo::nameEndsWith(const char* suffix) const
  {
    if (!suffix)
      return false;

    size_t suffix_len = strlen(suffix);

    if (_name_len < suffix_len)
      return false;

    return strcmp(_name + _name_len - suffix_len, suffix) == 0;
  }

  uint32_t FileInfo::getNameLen() const
  {
    return _name_len;
  }

  const char* FileInfo::getName() const
  {
    return _name;
  }

  bool FileInfo::operator<(const FileInfo& other) const
  {
    if (_is_dir != other._is_dir)
      return _is_dir;

    const char* lhs = _name;
    const char* rhs = other._name;

    while (*lhs && *rhs)
    {
      if (std::isdigit(*lhs) && std::isdigit(*rhs))
      {
        char* end_lhs;
        char* end_rhs;
        long num_lhs = std::strtol(lhs, &end_lhs, 10);
        long num_rhs = std::strtol(rhs, &end_rhs, 10);

        if (num_lhs != num_rhs)
        {
          return num_lhs < num_rhs;
        }
        lhs = end_lhs;
        rhs = end_rhs;
      }
      else
      {
        if (*lhs != *rhs)
        {
          return *lhs < *rhs;
        }
        ++lhs;
        ++rhs;
      }
    }

    return std::strcmp(lhs, rhs) < 0;
  }

  FileInfo::FileInfo(FileInfo&& other) noexcept : _name(other._name), _is_dir(other._is_dir), _name_len{other._name_len}
  {
    other._name = nullptr;
  }

  FileInfo& FileInfo::operator=(FileInfo&& other) noexcept
  {
    if (this != &other)
    {
      free(_name);
      _name = other._name;
      _is_dir = other._is_dir;
      _name_len = other._name_len;
      other._name = nullptr;
    }
    return *this;
  }
}  // namespace pixeler
