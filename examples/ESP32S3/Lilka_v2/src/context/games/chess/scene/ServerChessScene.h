#pragma once
#include "IChessScene.h"

namespace chess
{
  class ServerChessScene: public IChessScene
  {
  private:
  public:
    ServerChessScene();
    ~ServerChessScene();
  };

}  // namespace chess
