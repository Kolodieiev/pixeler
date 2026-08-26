#pragma once

#include "context/IContext.h"
#include "game/2D/IGameScene2D.h"
#include "game/DataStream.h"

namespace chess
{
  class IChessGameContext : public pixeler::IContext
  {
  public:
    IChessGameContext();
    virtual ~IChessGameContext();

  protected:
    virtual bool loop() = 0;
    virtual void update() = 0;

  protected:
    pixeler::DataStream _stored_objs{10};  // Об'єкт для перенесення даних між сценами
    pixeler::IGameScene2D* _scene{nullptr};
  };
}  // namespace chess
