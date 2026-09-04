#pragma once
#include "IChessScene.h"

namespace chess
{
  class AIChessScene : public IChessScene
  {
  public:
    explicit AIChessScene(DataStream& stored_objs);
    virtual ~AIChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) override;
  };
}  // namespace chess
