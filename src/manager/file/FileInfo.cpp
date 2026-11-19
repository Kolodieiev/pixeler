#pragma GCC optimize("O3")

#include "FileInfo.h"

#include <Arduino.h>

#include <cstring>

namespace pixeler
{
  static inline bool isAsciiDigit(unsigned char ch)
  {
    return ch >= '0' && ch <= '9';
  }

  FileInfo::FileInfo(const String& name, bool is_dir) : _name_len{name.length()}, _is_dir{is_dir}
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

    const size_t suffix_len = std::strlen(suffix);

    if (_name_len < suffix_len)
      return false;

    return std::memcmp(_name + _name_len - suffix_len, suffix, suffix_len) == 0;
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

    const unsigned char* lhs = reinterpret_cast<const unsigned char*>(_name);
    const unsigned char* rhs = reinterpret_cast<const unsigned char*>(other._name);

    while (*lhs && *rhs)
    {
      if (isAsciiDigit(*lhs) && isAsciiDigit(*rhs))
      {
        while (*lhs == '0')
          ++lhs;
        while (*rhs == '0')
          ++rhs;

        const unsigned char* num_lhs_start = lhs;
        while (isAsciiDigit(*lhs))
          ++lhs;

        const unsigned char* num_rhs_start = rhs;
        while (isAsciiDigit(*rhs))
          ++rhs;

        const size_t len_lhs = static_cast<size_t>(lhs - num_lhs_start);
        const size_t len_rhs = static_cast<size_t>(rhs - num_rhs_start);

        if (len_lhs != len_rhs)
          return len_lhs < len_rhs;

        if (len_lhs > 0)
        {
          const int cmp = std::memcmp(num_lhs_start, num_rhs_start, len_lhs);
          if (cmp != 0)
            return cmp < 0;
        }
      }
      else
      {
        if (*lhs != *rhs)
          return *lhs < *rhs;

        ++lhs;
        ++rhs;
      }
    }

    if (*lhs != *rhs)
      return *lhs < *rhs;

    return std::strcmp(_name, other._name) < 0;
  }

  FileInfo::FileInfo(FileInfo&& other) noexcept
      : _name{other._name}, _name_len{other._name_len}, _is_dir{other._is_dir}
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
