#pragma once

#include "game/IGameObject.h"
#include "game/ui/IGameUI.h"

using namespace pixeler;

namespace sokoban
{
  class SceneUI : public IGameUI
  {
  public:
    SceneUI() {}
    virtual ~SceneUI() {}
    virtual void onDraw() override;

  private:
  };
}  // namespace sokoban
