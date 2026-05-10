#pragma once
#pragma GCC optimize("O3")
#include <Arduino.h>

#include "input_config.h"
#include "bus/I2C_Bus.h"

#ifdef TOUCHSCREEN_SUPPORT

namespace pixeler
{
  class ITouchscreen
  {
  public:
    enum Swipe : uint8_t
    {
      SWIPE_NONE = 0,
      SWIPE_L,
      SWIPE_R,
      SWIPE_D,
      SWIPE_U,
    };

    enum Rotation : uint8_t
    {
      ROTATION_0 = 0,
      ROTATION_90,
      ROTATION_180,
      ROTATION_270
    };

    ITouchscreen();

    virtual ~ITouchscreen() {}

    virtual bool __begin() = 0;
    virtual void __update() = 0;

    void setRotation(Rotation rotation);

    void reset();

    bool isHolded() const;

    bool isPressed() const;

    bool isReleased() const;

    Swipe getSwipe() const;

    void lock(unsigned long lock_duration);

    uint16_t getTouchX() const;

    uint16_t getTouchY() const;

  protected:
    void processTouch();
    void setTouchPos(uint16_t x, uint16_t y);

  protected:
    unsigned long _lock_time{0};
    unsigned long _lock_duration{0};
    unsigned long _hold_duration{0};

    uint16_t _width{0};
    uint16_t _height{0};
    uint16_t _x_s{0};
    uint16_t _y_s{0};
    uint16_t _x_e{0};
    uint16_t _y_e{0};

    Swipe _swipe{SWIPE_NONE};

    uint8_t _rotation{ROTATION_0};
    uint8_t _chip_addr{0};

    bool _is_locked{false};
    bool _has_touch{false};
    bool _is_holded{false};
    bool _is_pressed{false};
    bool _is_released{false};
    bool _was_pressed{false};
  };
}  // namespace pixeler

#endif  // #ifdef TOUCHSCREEN_SUPPORT
