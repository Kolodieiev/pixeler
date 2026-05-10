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

  constexpr IWidget::TypeID ToggleSwitch::getTypeID()
  {
    return TypeID::TYPE_TOGGLE_SWITCH;
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
    if (_is_changed)
    {
      _is_changed = false;

      if (_visibility == INVISIBLE)
      {
        hide();
        return;
      }

      if (_is_on)
        _back_color = _on_color;
      else
        _back_color = _off_color;

      clear();

      //
      uint16_t lever_w;
      uint16_t lever_h;

      if (_width > _height)
      {
        lever_w = _height - 2;
        lever_h = lever_w - 4;
      }
      else
      {
        lever_h = _width - 2;
        lever_w = lever_h - 4;
      }

      uint16_t lever_x;
      uint16_t lever_y;

      if (_orientation == HORIZONTAL)
      {
        lever_x = _is_on ? _x_pos + _width - lever_w - 3 : _x_pos + 3;
        lever_y = _y_pos + 3;
      }
      else
      {
        lever_x = _x_pos + 3;
        lever_y = _is_on ? _y_pos + 3 : _y_pos + _height - lever_h - 3;
      }

      uint16_t x_offset{0};
      uint16_t y_offset{0};

      if (_parent)
      {
        x_offset = _parent->getXPos();
        y_offset = _parent->getYPos();
      }

      if (_corner_radius == 0)
        _display.fillRect(lever_x + x_offset, lever_y + y_offset, lever_w, lever_h, _lever_color);
      else
        _display.fillRoundRect(lever_x + x_offset, lever_y + y_offset, lever_w, lever_h, _corner_radius, _lever_color);
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
