#pragma once
#include "IChessScene.h"

namespace chess
{
  class SingleChessScene : public IChessScene
  {
  public:
    explicit SingleChessScene(DataStream& stored_objs);
    virtual ~SingleChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) override;
  };
}  // namespace chess
