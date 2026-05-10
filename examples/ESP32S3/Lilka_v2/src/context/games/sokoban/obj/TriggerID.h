// Перечислення ідентифікаторів тригерів, які можуть бути викорастані в сценах

#pragma once
#include <stdint.h>

namespace sokoban
{
  enum TriggerID : uint16_t
  {
    TRIGGER_NEXT_SCENE = 0,
    TRIGGER_GAME_FINISHED,
    TRIGGER_GAME_LOST,
  };
}
