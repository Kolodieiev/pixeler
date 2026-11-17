#pragma GCC optimize("O3")
#include "ProgressBar.h"

namespace pixeler
{
  ProgressBar::ProgressBar(uint16_t widget_ID) : IWidget(widget_ID, TYPE_PROGRESSBAR) {}

  ProgressBar::~ProgressBar() {}

  void ProgressBar::setMax(uint32_t max)
  {
    if (max < 1)
      _max_val = 1;
    else
      _max_val = max;

    _is_changed = true;
  }

  void ProgressBar::setProgress(uint32_t progress)
  {
    if (progress > _max_val)
      _progress = _max_val;
    else if (progress < 1)
      _progress = 1;
    else
      _progress = progress;

    _is_changed = true;
  }

  void ProgressBar::setProgressColor(uint16_t color)
  {
    _progress_color = color;
    _is_changed = true;
  }

  void ProgressBar::setOrientation(Orientation orientation)
  {
    _orientation = orientation;
    _is_changed = true;
  }

  void ProgressBar::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    ProgressBar* clone = static_cast<ProgressBar*>(widget);
    clone->_progress = _progress;
    clone->_max_val = _max_val;
    clone->_progress_color = _progress_color;
    clone->_orientation = _orientation;
    clone->_prev_progress = _prev_progress;
  }

  ProgressBar* ProgressBar::clone(uint16_t id) const
  {
    try
    {
      ProgressBar* clone = new ProgressBar(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void ProgressBar::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      _is_first_draw = true;
      hide();
      return;
    }

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    if (_orientation == HORIZONTAL)
    {
      uint16_t progress_w = static_cast<uint32_t>(_width) * _progress / _max_val;
      if (progress_w < 3)
        progress_w = 3;

      if (!_is_first_draw)
      {
        uint16_t prev_prgrs_pos = static_cast<uint32_t>(_width) * _prev_progress / _max_val;
        if (prev_prgrs_pos < 3)
          prev_prgrs_pos = 3;

        if (progress_w > prev_prgrs_pos)  // Заливка тільки прогресу
        {
          _display.fillRect(abs_x + prev_prgrs_pos - 1,
                            abs_y + 1,
                            progress_w - prev_prgrs_pos,
                            _height - 2,
                            _progress_color);
        }
        else if (progress_w < prev_prgrs_pos)  // Заливка тільки фону
        {
          _display.fillRect(abs_x + progress_w - 1,
                            abs_y + 1,
                            prev_prgrs_pos - progress_w,
                            _height - 2,
                            _back_color);
        }
      }
      else
      {
        _display.drawRect(abs_x,
                          abs_y,
                          _width,
                          _height,
                          _border_color);  // рамка

        _display.fillRect(abs_x + 1,
                          abs_y + 1,
                          progress_w - 2,
                          _height - 2,
                          _progress_color);  // прогрес

        _display.fillRect(abs_x + progress_w - 1,
                          abs_y + 1,
                          _width - progress_w,
                          _height - 2,
                          _back_color);  // фон

        _is_first_draw = false;
      }
    }
    else  // orientation == vertical
    {
      uint16_t progress_h = static_cast<uint32_t>(_height) * _progress / _max_val;
      if (progress_h < 3)
        progress_h = 3;

      if (!_is_first_draw)
      {
        uint16_t prev_prgrs_pos = static_cast<uint32_t>(_height) * _prev_progress / _max_val;
        if (prev_prgrs_pos < 3)
          prev_prgrs_pos = 3;

        if (progress_h > prev_prgrs_pos)  // Заливка тільки прогресу
        {
          _display.fillRect(abs_x + 1,
                            abs_y + prev_prgrs_pos - 1,
                            _width - 2,
                            progress_h - prev_prgrs_pos,
                            _progress_color);
        }
        else if (progress_h < prev_prgrs_pos)  // Заливка тільки фону
        {
          _display.fillRect(abs_x + 1,
                            abs_y + progress_h - 1,
                            _width - 2,
                            prev_prgrs_pos - progress_h,
                            _back_color);
        }
      }
      else
      {
        _display.drawRect(abs_x,
                          abs_y,
                          _width,
                          _height,
                          _border_color);  // рамка

        _display.fillRect(abs_x + 1,
                          abs_y + 1,
                          _width - 2,
                          progress_h - 2,
                          _progress_color);  // прогрес

        _display.fillRect(abs_x + 1,
                          abs_y + progress_h - 1,
                          _width - 2,
                          _height - progress_h,
                          _back_color);  // фон

        _is_first_draw = false;
      }
    }

    _prev_progress = _progress;
  }

  IWidget::Orientation ProgressBar::getOrientation() const
  {
    return _orientation;
  }

  void ProgressBar::reset()
  {
    _progress = 1;
    _prev_progress = 1;

    if (_visibility == INVISIBLE || _is_first_draw)
      return;

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    if (_orientation == HORIZONTAL)
    {
      _display.fillRect(abs_x + 2,
                        abs_y + 1,
                        _width - 3,
                        _height - 2,
                        _back_color);
    }
    else
    {
      _display.fillRect(abs_x + 1,
                        abs_y + 2,
                        _width - 2,
                        _height - 3,
                        _back_color);
    }
  }

  uint32_t ProgressBar::getProgress() const
  {
    return _progress;
  }

  uint32_t ProgressBar::getProgressAt(uint16_t x, uint16_t y) const
  {
    if (_max_val == 1)
      return 0;

    if (_orientation == HORIZONTAL)
    {
      const uint16_t x_pos = getXPos();

      if (x >= x_pos + _width || x <= x_pos)
        return 0;

      const uint32_t relative_x = x - x_pos;
      return relative_x * _max_val / _width;
    }
    else
    {
      const uint16_t y_pos = getYPos();

      if (y >= y_pos + _height || y <= y_pos)
        return 0;

      const uint32_t relative_y = y - y_pos;
      return relative_y * _max_val / _height;
    }
  }

  uint32_t ProgressBar::getMax() const
  {
    return _max_val;
  }
}  // namespace pixeler
