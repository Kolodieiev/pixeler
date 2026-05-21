#pragma once

#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class KnightObj : public IPiece
  {
  public:
    KnightObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~KnightObj();

  private:
  };
}  // namespace chess
