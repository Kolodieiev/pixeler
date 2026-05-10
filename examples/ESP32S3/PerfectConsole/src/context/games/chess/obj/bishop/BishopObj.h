#pragma once

#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class BishopObj : public IPiece
  {
  public:
    BishopObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~BishopObj();

  private:
  };
}  // namespace chess
