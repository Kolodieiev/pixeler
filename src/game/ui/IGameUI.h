#pragma once
#include "defines.h"
#include "driver/graphics/DisplayWrapper.h"

namespace pixeler
{
  class IGameUI
  {
  public:
    IGameUI() {}
    virtual ~IGameUI() {}
    virtual void onDraw() = 0;
    IGameUI(const IGameUI& rhs) = delete;
    IGameUI& operator=(const IGameUI& rhs) = delete;
  };
}  // namespace pixeler
