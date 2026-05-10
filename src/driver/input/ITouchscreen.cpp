#pragma GCC optimize("O3")
#include "ITouchscreen.h"

#ifdef TOUCHSCREEN_SUPPORT

namespace pixeler
{
  ITouchscreen::ITouchscreen() : _width{TOUCH_WIDTH}, _height{TOUCH_HEIGHT}
  {
  }

  void ITouchscreen::setRotation(Rotation rotation)
  {
    if (rotation > ROTATION_270)
      rotation = ROTATION_0;

    _rotation = rotation;
  }

  void ITouchscreen::reset()
  {
    _is_holded = false;
    _is_pressed = false;
    _is_released = false;
    _has_touch = false;
    _swipe = SWIPE_NONE;
    _x_s = 0;
    _y_s = 0;
    _x_e = 0;
    _y_e = 0;
  }

  void ITouchscreen::lock(unsigned long lock_duration)
  {
    _is_locked = true;
    _is_holded = false;
    _is_released = false;
    _is_pressed = false;
    _has_touch = false;
    _swipe = SWIPE_NONE;
    _lock_duration = lock_duration;
    _lock_time = millis();
  }

  void ITouchscreen::processTouch()
  {
    if (_was_pressed)
    {
      _was_pressed = false;
      return;
    }

    if (!_is_holded)
    {
      if (_has_touch)
      {
        _has_touch = false;
        _is_released = true;

        int x_diff = _x_s - _x_e;
        int y_diff = _y_s - _y_e;

        if (__builtin_abs(x_diff) > 30 || __builtin_abs(y_diff) > 30)
        {
          if (__builtin_abs(x_diff) >= __builtin_abs(y_diff))
          {
            _swipe = (_x_s > _x_e) ? SWIPE_R : SWIPE_L;
          }
          else
          {
            _swipe = (_y_s > _y_e) ? SWIPE_D : SWIPE_U;
          }
        }
      }
      return;
    }

    if (_has_touch)
    {
      if (!_is_pressed && (millis() - _hold_duration > PRESS_DURATION))
      {
        _has_touch = false;
        _is_pressed = true;
        _was_pressed = true;
      }
    }
    else
    {
      _has_touch = true;
      _is_released = false;
      _hold_duration = millis();
      // Встановлюємо стартові координати в момент першого торкання
      _x_s = _x_e;
      _y_s = _y_e;

      // log_i("Touch: X=%d, Y=%d", _x_s, _y_s);
    }
  }

  void ITouchscreen::setTouchPos(uint16_t x, uint16_t y)
  {
    uint16_t temp;

    switch (_rotation)
    {
      case ROTATION_90:
        temp = x;
        x = _height - y;
        y = temp;
        break;
      case ROTATION_180:
        x = _width - x;
        y = _height - y;
        break;
      case ROTATION_270:
        temp = x;
        x = y;
        y = _width - temp;
        break;
      default:
        break;
    }

    _x_e = x;
    _y_e = y;
    if (!_is_holded)
    {  // Якщо це початок дотику
      _x_s = x;
      _y_s = y;
    }
  }

  uint16_t ITouchscreen::getTouchX() const
  {
    return _x_s;
  }

  uint16_t ITouchscreen::getTouchY() const
  {
    return _y_s;
  }

  bool ITouchscreen::isHolded() const
  {
    return _is_holded;
  }

  bool ITouchscreen::isPressed() const
  {
    return _is_pressed;
  }

  bool ITouchscreen::isReleased() const
  {
    return _is_released;
  }

  ITouchscreen::Swipe ITouchscreen::getSwipe() const
  {
    return _swipe;
  }

}  // namespace pixeler

#endif  // #ifdef TOUCHSCREEN_SUPPORT
