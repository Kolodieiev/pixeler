#pragma once
#pragma GCC optimize("O3")
#include "defines.h"

namespace pixeler
{
  /**
   * @brief Обрізає розширення файлу в рядку.
   *
   * @param filename Рядок, в якому потрібно видалити розширення.
   */
  void rmFilenameExt(String& filename);

  bool startsWith(const char* base, const char* str);
  void trim(char* str);
  void strlower(char* str);
  bool endsWith(const char* base, const char* searchString);
  int indexOf(const char* base, const char* str, int startIndex = 0);
  int indexOf(const char* base, char ch, int startIndex = 0);
  int lastIndexOf(const char* haystack, const char* needle);
  int lastIndexOf(const char* haystack, const char needle);
}  // namespace pixeler
