#pragma GCC optimize("O3")
#include "ProgressBar.h"

namespace pixeler
{
  ProgressBar::ProgressBar(uint16_t widget_ID) : IWidget(widget_ID, TYPE_PROGRESSBAR) {}

  ProgressBar::~ProgressBar() {}

  void ProgressBar::setMax(uint32_t max)
  {
    if (max < 1)
      _max = 1;
    else
      _max = max;

    _is_changed = true;
  }

  void ProgressBar::setProgress(uint32_t progress)
  {
    if (progress > _max)
      _progress = _max;
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
    clone->_max = _max;
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
      hide();
      return;
    }

    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
    }

    if (_orientation == HORIZONTAL)
    {
      uint16_t progressW = static_cast<uint32_t>(_width) * _progress / _max;
      if (progressW < 3)
        progressW = 3;

      if (!_is_first_draw)
      {
        uint16_t next_prgrs_pos = static_cast<uint32_t>(_width) * _prev_progress / _max;

        if (_progress > _prev_progress)  // Заливка тільки прогресу
        {
          uint16_t x = _x_pos + x_offset + next_prgrs_pos;
          if (next_prgrs_pos < 2)
            ++x;
          else
            --x;

          _display.fillRect(x,
                            _y_pos + y_offset + 1,
                            progressW - next_prgrs_pos,
                            _height - 2,
                            _progress_color);
        }
        else if (_progress < _prev_progress)  // Заливка тільки фону
        {
          _display.fillRect(_x_pos + x_offset + progressW - 1,
                            _y_pos + y_offset + 1,
                            next_prgrs_pos - progressW,
                            _height - 2,
                            _back_color);
        }
      }
      else
      {
        _display.drawRect(_x_pos + x_offset,
                          _y_pos + y_offset,
                          _width,
                          _height,
                          _border_color);  // рамка

        _display.fillRect(_x_pos + x_offset + 1,
                          _y_pos + y_offset + 1,
                          progressW - 2,
                          _height - 2,
                          _progress_color);  // прогрес

        _display.fillRect(_x_pos + x_offset + 1 + progressW - 2,
                          _y_pos + y_offset + 1,
                          _width - progressW,
                          _height - 2,
                          _back_color);  // фон

        _is_first_draw = false;
      }
    }
    else  // orientation == vertical
    {
      uint16_t progressH = static_cast<uint32_t>(_height) * _progress / _max;
      if (progressH < 3)
        progressH = 3;

      if (!_is_first_draw)
      {
        uint16_t next_prgrs_pos = static_cast<uint32_t>(_height) * _prev_progress / _max;

        if (_progress > _prev_progress)  // Заливка тільки прогресу
        {
          uint16_t y = _y_pos + y_offset + next_prgrs_pos;
          if (next_prgrs_pos < 2)
            ++y;
          else
            --y;

          _display.fillRect(_x_pos + x_offset + 1,
                            y,
                            _width - 2,
                            progressH - next_prgrs_pos + 1,
                            _progress_color);
        }
        else if (_progress < _prev_progress)  // Заливка тільки фону
        {
          _display.fillRect(_x_pos + x_offset + 1,
                            _y_pos + y_offset + progressH - 1,
                            _width - 2,
                            next_prgrs_pos - progressH,
                            _back_color);
        }
      }
      else
      {
        _display.drawRect(_x_pos + x_offset,
                          _y_pos + y_offset,
                          _width,
                          _height,
                          _border_color);  // рамка

        _display.fillRect(_x_pos + x_offset + 1,
                          _y_pos + y_offset + 1,
                          _width - 2,
                          progressH - 2,
                          _progress_color);  // прогрес

        _display.fillRect(_x_pos + x_offset + 1,
                          _y_pos + y_offset + progressH - 2,
                          _width - 2,
                          _height - progressH + 1,
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
    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
    }

    _progress = 1;

    if (_orientation == HORIZONTAL)
    {
      _display.fillRect(_x_pos + x_offset + 3,
                        _y_pos + y_offset + 1,
                        _width - 4,
                        _height - 2,
                        _back_color);
    }
    else
    {
      _display.fillRect(_x_pos + x_offset + 1,
                        _y_pos + y_offset + 3,
                        _width - 2,
                        _height - 4,
                        _back_color);
    }
  }

  uint32_t ProgressBar::getProgress() const
  {
    return _progress;
  }

  uint32_t ProgressBar::getProgressAt(uint16_t x, uint16_t y) const
  {
    if (_max == 1)
      return 0;

    if (_orientation == HORIZONTAL)
    {
      uint16_t x_pos = getXPos();

      if (x >= x_pos + _width || x <= x_pos)
        return 0;

      uint32_t relative_x = x - x_pos;
      return relative_x * _max / _width;
    }
    else
    {
      uint16_t y_pos = getYPos();
      if (y >= getYPos() + _height || y <= y_pos)
        return 0;

      uint32_t relative_y = y - y_pos;
      return relative_y * _max / _height;
    }
  }

  uint32_t ProgressBar::getMax() const
  {
    return _max;
  }
}  // namespace pixeler
