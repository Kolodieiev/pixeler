#pragma once
#pragma GCC optimize("O3")
#include <Arduino.h>

#include "../ITouchscreen.h"

#ifdef AXS15231B_DRIVER

namespace pixeler
{
  class AXS15231B : public ITouchscreen
  {
  public:
    AXS15231B() {}
    virtual ~AXS15231B() {}
    virtual bool __begin() override;
    virtual void __update() override;

  private:
    void resetChip();

  private:
  };
}  // namespace pixeler

#endif  // AXS15231B_DRIVER
