#pragma once

#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class QueenObj : public IPiece
  {
  public:
    QueenObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~QueenObj();

  private:
  };
}  // namespace chess
