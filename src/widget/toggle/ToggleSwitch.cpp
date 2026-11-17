#pragma GCC optimize("O3")
#include "ToggleSwitch.h"

namespace pixeler
{
  void ToggleSwitch::setOrientation(Orientation orientation)
  {
    _orientation = orientation;
    _is_changed = true;
  }

  IWidget::Orientation ToggleSwitch::getOrientation() const
  {
    return _orientation;
  }

  void ToggleSwitch::setOffColor(uint16_t color)
  {
    _off_color = color;
    _is_changed = true;
  }

  uint16_t ToggleSwitch::getOffColor() const
  {
    return _off_color;
  }

  void ToggleSwitch::setLeverColor(uint16_t color)
  {
    _lever_color = color;
    _is_changed = true;
  }

  uint16_t ToggleSwitch::getLeverColor() const
  {
    return _lever_color;
  }

  void ToggleSwitch::setOnColor(uint16_t color)
  {
    _on_color = color;
    _is_changed = true;
  }

  uint16_t ToggleSwitch::getOnColor() const
  {
    return _on_color;
  }

  bool ToggleSwitch::isOn() const
  {
    return _is_on;
  }

  void ToggleSwitch::setOn(bool state)
  {
    _is_on = state;
    _is_changed = true;
  }

  void ToggleSwitch::toggle()
  {
    _is_on = !_is_on;
    _is_changed = true;
  }

  void ToggleSwitch::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    _back_color = _is_on ? _on_color : _off_color;

    clear();

    if (_width < 7 || _height < 7)
      return;

    uint16_t lever_w;
    uint16_t lever_h;

    if (_width > _height)
    {
      lever_w = _height - 2;
      lever_h = _height - 6;
    }
    else
    {
      lever_h = _width - 2;
      lever_w = _width - 6;
    }

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    uint16_t lever_x;
    uint16_t lever_y;

    if (_orientation == HORIZONTAL)
    {
      lever_x = _is_on ? (abs_x + _width - lever_w - 3) : (abs_x + 3);
      lever_y = abs_y + 3;
    }
    else
    {
      lever_x = abs_x + 3;
      lever_y = _is_on ? (abs_y + 3) : (abs_y + _height - lever_h - 3);
    }

    if (_corner_radius == 0)
    {
      _display.fillRect(lever_x, lever_y, lever_w, lever_h, _lever_color);
    }
    else
    {
      const uint8_t lever_radius = (_corner_radius > 2) ? (_corner_radius - 2) : 1;
      _display.fillRoundRect(lever_x, lever_y, lever_w, lever_h, lever_radius, _lever_color);
    }
  }

  void ToggleSwitch::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    ToggleSwitch* clone = static_cast<ToggleSwitch*>(widget);

    clone->_lever_color = _lever_color;
    clone->_on_color = _on_color;
    clone->_off_color = _off_color;
    clone->_orientation = _orientation;
    clone->_is_on = _is_on;
  }

  ToggleSwitch* ToggleSwitch::clone(uint16_t id) const
  {
    try
    {
      ToggleSwitch* clone = new ToggleSwitch(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }
}  // namespace pixeler
