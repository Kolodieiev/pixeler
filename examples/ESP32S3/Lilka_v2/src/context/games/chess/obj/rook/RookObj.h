#pragma once

#include "../ICastlable.h"
#include "../IPiece.h"

using namespace pixeler;

namespace chess
{
  class RookObj : public IPiece, public ICastlable
  {
  public:
    RookObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~RookObj();

  private:
  };
}  // namespace chess
