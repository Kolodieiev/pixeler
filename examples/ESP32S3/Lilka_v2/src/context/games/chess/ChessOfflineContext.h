#pragma once

#include "IChessGameContext.h"

namespace chess
{
  class ChessOfflineContext : public IChessGameContext
  {
  public:
    ChessOfflineContext(uint8_t player_num);
    virtual ~ChessOfflineContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
  };
}  // namespace chess
