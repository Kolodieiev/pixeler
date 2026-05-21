#pragma once

#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class PawnObj : public IPiece
  {
  public:
    PawnObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~PawnObj();

    void turnIntoQueen();
    bool isQueen() const;

  private:
    bool _is_queen{false};
  };
}  // namespace chess

// TODO перевірка пертворення на королеву у момент завершення ходу
