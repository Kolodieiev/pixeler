#include "ScrollBar.h"

namespace pixeler
{
  ScrollBar::ScrollBar(uint16_t widget_ID) : IWidget(widget_ID, TYPE_SCROLLBAR) {}

  void ScrollBar::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    ScrollBar* clone = static_cast<ScrollBar*>(widget);
    clone->_max_value = _max_value;
    clone->_cur_value = _cur_value;
    clone->_slider_color = _slider_color;
    clone->_orientation = _orientation;
    clone->_slider_width = _slider_width;
    clone->_slider_height = _slider_height;
    clone->_slider_step_size = _slider_step_size;
    clone->_smart_scroll_enabled = _smart_scroll_enabled;
    clone->_smart_value = _smart_value;
    clone->_steps_to_scroll = _steps_to_scroll;
  }

  ScrollBar* ScrollBar::clone(uint16_t id) const
  {
    try
    {
      ScrollBar* clone = new ScrollBar(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void ScrollBar::setValue(uint16_t value)
  {
    const uint16_t max_index = _max_value - 1;
    _cur_value = (value > max_index) ? max_index : value;

    if (_smart_scroll_enabled)
      _smart_value = _cur_value / _steps_to_scroll;

    _is_changed = true;
  }

  void ScrollBar::setOrientation(Orientation orientation)
  {
    _orientation = orientation;
    _is_changed = true;
  }

  IWidget::Orientation ScrollBar::getOrientation() const
  {
    return _orientation;
  }

  void ScrollBar::setSliderColor(uint16_t color)
  {
    _slider_color = color;
    _is_changed = true;
  }

  uint16_t ScrollBar::getValue() const
  {
    return _cur_value;
  }

  uint16_t ScrollBar::getMax() const
  {
    return _max_value;
  }

  void ScrollBar::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();
    const uint16_t step_value = _smart_scroll_enabled ? _smart_value : _cur_value;
    const uint16_t offset = _slider_step_size * step_value;

    if (_orientation == VERTICAL)
    {
      if (offset > 0)
        _display.fillRect(abs_x, abs_y, _width, offset, _back_color);

      _display.fillRect(abs_x, abs_y + offset, _slider_width, _slider_height, _slider_color);

      const uint16_t after_offset = offset + _slider_height;
      if (after_offset < _height)
        _display.fillRect(abs_x, abs_y + after_offset, _width, _height - after_offset, _back_color);
    }
    else
    {
      if (offset > 0)
        _display.fillRect(abs_x, abs_y, offset, _height, _back_color);

      _display.fillRect(abs_x + offset, abs_y, _slider_width, _slider_height, _slider_color);

      const uint16_t after_offset = offset + _slider_width;
      if (after_offset < _width)
        _display.fillRect(abs_x + after_offset, abs_y, _width - after_offset, _height, _back_color);
    }
  }

  void ScrollBar::setMax(uint16_t max_value)
  {
    _max_value = (max_value > 0) ? max_value : 1;

    if (_cur_value >= _max_value)
      _cur_value = _max_value - 1;

    const uint16_t track_len = (_orientation == VERTICAL) ? _height : _width;
    uint16_t slider_len;

    if (track_len >= _max_value)
    {
      _smart_scroll_enabled = false;
      _slider_step_size = track_len / _max_value;
      slider_len = _slider_step_size + (track_len - (_slider_step_size * _max_value));
    }
    else
    {
      _smart_scroll_enabled = true;
      _slider_step_size = 1;
      _steps_to_scroll = (_max_value + track_len - 1) / track_len;
      const uint16_t positions_number = (_max_value - 1) / _steps_to_scroll;
      slider_len = track_len - positions_number;

      _smart_value = _cur_value / _steps_to_scroll;
    }

    if (_orientation == VERTICAL)
    {
      _slider_width = _width;
      _slider_height = slider_len;
    }
    else
    {
      _slider_width = slider_len;
      _slider_height = _height;
    }

    _is_changed = true;
  }

  bool ScrollBar::scrollDown()
  {
    if (_cur_value >= _max_value - 1)
      return false;

    ++_cur_value;

    if (_smart_scroll_enabled)
    {
      const uint16_t next_smart = _cur_value / _steps_to_scroll;
      if (next_smart != _smart_value)
      {
        _smart_value = next_smart;
        _is_changed = true;
      }
    }
    else
    {
      _is_changed = true;
    }

    return true;
  }

  bool ScrollBar::scrollUp()
  {
    if (_cur_value == 0)
      return false;

    --_cur_value;

    if (_smart_scroll_enabled)
    {
      const uint16_t next_smart = _cur_value / _steps_to_scroll;
      if (next_smart != _smart_value)
      {
        _smart_value = next_smart;
        _is_changed = true;
      }
    }
    else
    {
      _is_changed = true;
    }

    return true;
  }
}  // namespace pixeler
