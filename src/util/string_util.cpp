#pragma GCC optimize("O3")
#include "string_util.h"

namespace pixeler
{
  void rmFilenameExt(String& filename)
  {
    int pos = filename.lastIndexOf(".");
    if (pos != -1)
      filename.remove(pos);
  }

  bool startsWith(const char* base, const char* str)
  {
    char c;
    while ((c = *str++) != '\0')
      if (c != *base++)
        return false;
    return true;
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

  bool endsWith(const char* base, const char* searchString)
  {
    int32_t slen = strlen(searchString);
    if (slen == 0)
      return false;
    const char* p = base + strlen(base);
    p -= slen;
    if (p < base)
      return false;
    return (strncmp(p, searchString, slen) == 0);
  }

  int indexOf(const char* base, const char* str, int startIndex)
  {
    const char* p = base;
    for (; startIndex > 0; startIndex--)
      if (*p++ == '\0')
        return -1;
    char* pos = strstr(p, str);
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
    char* pos = strchr(p, ch);
    if (pos == nullptr)
      return -1;
    return pos - base;
  }

  int lastIndexOf(const char* haystack, const char* needle)
  {
    int nlen = strlen(needle);
    if (nlen == 0)
      return -1;
    const char* p = haystack - nlen + strlen(haystack);
    while (p >= haystack)
    {
      int i = 0;
      while (needle[i] == p[i])
        if (++i == nlen)
          return p - haystack;
      p--;
    }
    return -1;
  }

  int lastIndexOf(const char* haystack, const char needle)
  {
    const char* p = strrchr(haystack, needle);
    return (p ? p - haystack : -1);
  }
}  // namespace pixeler
