#include "string_util.h"

#include <cstring>

namespace pixeler
{
  void rmFilenameExt(String& filename)
  {
    const int slash_pos = filename.lastIndexOf('/');
    const int dot_pos = filename.lastIndexOf('.');
    if (dot_pos > slash_pos)
      filename.remove(dot_pos);
  }

  bool startsWith(const char* base, const char* str)
  {
    char c;
    while ((c = *str++) != '\0')
      if (c != *base++)
        return false;
    return true;
  }

  bool isEmptyStr(const char* str)
  {
    return !str || *str == '\0';
  }

  void trim(char* str)
  {
    char* start = str;
    char* end;
    while (isspace((unsigned char)*start))
      start++;

    if (*start == 0)
    {
      str[0] = '\0';
      return;
    }

    end = start + strlen(start) - 1;

    while (end > start && isspace((unsigned char)*end))
      end--;
    end[1] = '\0';

    memmove(str, start, strlen(start) + 1);
  }

  void strlower(char* str)
  {
    unsigned char* p = (unsigned char*)str;
    while (*p)
    {
      *p = tolower((unsigned char)*p);
      p++;
    }
  }

  bool endsWith(const char* base, const char* suffix)
  {
    const size_t suffix_len = std::strlen(suffix);
    if (suffix_len == 0)
      return false;

    const size_t base_len = std::strlen(base);
    if (base_len < suffix_len)
      return false;

    return std::memcmp(base + base_len - suffix_len, suffix, suffix_len) == 0;
  }

  int indexOf(const char* base, const char* str, int startIndex)
  {
    const char* p = base;
    for (; startIndex > 0; startIndex--)
      if (*p++ == '\0')
        return -1;
    const char* pos = strstr(p, str);
    if (pos == nullptr)
      return -1;
    return pos - base;
  }

  int indexOf(const char* base, char ch, int startIndex)
  {
    const char* p = base;
    for (; startIndex > 0; startIndex--)
      if (*p++ == '\0')
        return -1;
    const char* pos = strchr(p, ch);
    if (pos == nullptr)
      return -1;
    return pos - base;
  }

  int lastIndexOf(const char* haystack, const char* needle)
  {
    const size_t nlen = std::strlen(needle);
    if (nlen == 0)
      return -1;

    const size_t hlen = std::strlen(haystack);
    if (hlen < nlen)
      return -1;

    for (size_t i = hlen - nlen + 1; i-- > 0;)
    {
      if (haystack[i] == needle[0] && std::memcmp(haystack + i, needle, nlen) == 0)
        return static_cast<int>(i);
    }

    return -1;
  }

  int lastIndexOf(const char* haystack, const char needle)
  {
    const char* p = strrchr(haystack, needle);
    return (p ? p - haystack : -1);
  }
}  // namespace pixeler
