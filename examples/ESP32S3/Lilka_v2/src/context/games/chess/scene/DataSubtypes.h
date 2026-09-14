#pragma once
#include <cstdint>

namespace chess
{
  enum DataSubtypes : uint8_t
  {
    SUBTYPE_MOVE_U,
    SUBTYPE_MOVE_D,
    SUBTYPE_MOVE_L,
    SUBTYPE_MOVE_R,
    SUBTYPE_MOVE_OK,
    SUBTYPE_CLEAR_SELECT,
    SUBTYPE_MAIN_CLIENT,
  };
}  // namespace chess
