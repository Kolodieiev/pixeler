#pragma once
#pragma GCC optimize("O3")
#include <Arduino.h>

#include "../ITouchscreen.h"

#ifdef GT911_DRIVER

namespace pixeler
{
  class GT911 : public ITouchscreen
  {
  public:
    GT911();
    virtual ~GT911() {}

    virtual bool __begin() override;
    virtual void __update() override;

  private:
    void resetChip();

    void calculateChecksum(uint8_t* out_conf_buf);
    void reflashConfig(uint8_t* out_conf_buf);
    void setResolution(uint8_t* out_conf_buf, uint16_t width, uint16_t height);
  };
}  // namespace pixeler

#endif  // GT911_DRIVER
