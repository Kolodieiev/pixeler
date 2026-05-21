// Перечислення, в якому зберігаються ідентифікатори типів ігрових об'єктів

#pragma once
#include <stdint.h>

namespace chess
{
  enum TypeID : uint16_t
  {
    TYPE_NONE = 0,
    TYPE_PLAYER,
    TYPE_BISHOP,
    TYPE_QUEEN,
    TYPE_KING,
    TYPE_KNIGHT,
    TYPE_PAWN,
    TYPE_ROOK,
  };
}  // namespace chess
