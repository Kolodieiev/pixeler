// Перечислення, в якому зберігаються ідентифікатори типів ігрових об'єктів

#pragma once
#include <stdint.h>

namespace sokoban
{
  enum TypeID : uint16_t
  {
    TYPE_NONE = 0,
    TYPE_HERO,
    TYPE_BOX,
    TYPE_BOX_POINT
  };
}
