#pragma once

#include "../ICastlable.h"
#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class KingObj : public IPiece, public ICastlable
  {
  public:
    KingObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~KingObj();

  private:
  };
}  // namespace chess
