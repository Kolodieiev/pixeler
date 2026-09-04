#pragma once

#include "IChessGameContext.h"
#include "scene/IChessScene.h"

namespace chess
{
  class LocalChessContext : public IChessGameContext
  {
  public:
    LocalChessContext(uint8_t player_num);
    virtual ~LocalChessContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
  };
}  // namespace chess
