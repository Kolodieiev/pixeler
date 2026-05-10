#pragma GCC optimize("O3")
#include "IWidget.h"

namespace pixeler
{
  IWidget::IWidget(uint16_t widget_ID, TypeID type_ID, bool is_container) : _id{widget_ID},
                                                                            _type_ID{type_ID}

  {
    if (widget_ID == 0)
    {
      log_e("Ідентифікатор віджета повинен бути більшим за 0");
      esp_restart();
    }
  }

  IWidget::~IWidget() {}

  IWidget::TypeID IWidget::getTypeID() const
  {
    return _type_ID;
  }

  void IWidget::drawForced()
  {
    _is_changed = true;
    onDraw();
  }

  void IWidget::setPos(uint16_t x, uint16_t y)
  {
    _x_pos = x;
    _y_pos = y;
    _is_changed = true;
  }

  void IWidget::setHeight(uint16_t height)
  {
    _height = height;
    _is_changed = true;
  }

  void IWidget::setWidth(uint16_t width)
  {
    _width = width;
    _is_changed = true;
  }

  void IWidget::setBackColor(uint16_t back_color)
  {
    _back_color = back_color;
    _is_changed = true;
  }

  void IWidget::setParent(IWidget* parent)
  {
    _parent = parent;
    _is_changed = true;
  }

  const IWidget* IWidget::getParent() const
  {
    return _parent;
  }

  void IWidget::setCornerRadius(uint8_t radius)
  {
    _corner_radius = radius;
    _is_changed = true;
  }

  void IWidget::setBorder(bool state)
  {
    _has_border = state;
    _is_changed = true;
  }

  void IWidget::setBorderColor(uint16_t color)
  {
    _border_color = color;
    _is_changed = true;
  }

  void IWidget::clear(bool keep_border)
  {
    if (_is_transparent)
      return;

    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
    }

    if (_corner_radius == 0)
    {
#if CONFIG_IDF_TARGET_ESP32P4
      if (_width * _height > PPA_FILL_SIZE_TRIGG)
      {
        bool old_state = _display.isPPAEnabled();

        _display.setPPAState(true);
        _display.fillRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _back_color);
        _display.setPPAState(old_state);
      }
      else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
      {
        _display.fillRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _back_color);
      }

      if (keep_border && _has_border)
        _display.drawRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _border_color);
    }
    else
    {
      _display.fillRoundRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _corner_radius, _back_color);

      if (keep_border && _has_border)
      {
        _display.drawRoundRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _corner_radius, _border_color);
      }
      else if (_need_clear_border)
      {
        _need_clear_border = false;
        _display.drawRoundRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _corner_radius, _back_color);
      }
    }
  }

  void IWidget::hide()
  {
    if (_is_transparent)
      return;

    uint16_t back_color{0};
    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
      back_color = _parent->getBackColor();
    }

    if (_corner_radius == 0)
      _display.fillRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, back_color);
    else
    {
      if (_has_border || _need_clear_border)
      {
        _need_clear_border = false;
        _display.drawRoundRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _corner_radius, back_color);
      }
      _display.fillRoundRect(_x_pos + x_offset, _y_pos + y_offset, _width, _height, _corner_radius, back_color);
    }
  }

  void IWidget::copyTo(IWidget* widget) const
  {
    if (!widget)
    {
      log_e("Спроба копіювати властивості (id: %u) до nullptr", _id);
      esp_restart();
    }

    widget->_parent = _parent;
    widget->_x_pos = _x_pos;
    widget->_y_pos = _y_pos;
    widget->_width = _width;
    widget->_height = _height;
    widget->_back_color = _back_color;
    widget->_border_color = _border_color;
    widget->_focus_border_color = _focus_border_color;
    widget->_old_border_color = _old_border_color;
    widget->_focus_back_color = _focus_back_color;
    widget->_old_back_color = _old_back_color;
    widget->_visibility = _visibility;
    widget->_corner_radius = _corner_radius;
    widget->_is_touchable = _is_touchable;
    widget->_is_changed = _is_changed;
    widget->_has_border = _has_border;
    widget->_is_transparent = _is_transparent;
    widget->_has_focus = _has_focus;
    widget->_old_border_state = _old_border_state;
    widget->_need_clear_border = _need_clear_border;
    widget->_need_change_border = _need_change_border;
    widget->_need_change_back = _need_change_back;
  }

  uint16_t IWidget::getXPos() const
  {
    if (_parent)
      return _parent->getXPos() + _x_pos;
    else
      return _x_pos;
  }

  uint16_t IWidget::getYPos() const
  {
    if (_parent)
      return _parent->getYPos() + _y_pos;
    else
      return _y_pos;
  }

  uint16_t IWidget::getXPosLoc() const
  {
    return _x_pos;
  }

  uint16_t IWidget::getYPosLoc() const
  {
    return _y_pos;
  }

  uint8_t IWidget::getCornerRadius() const
  {
    return _corner_radius;
  }

  uint16_t IWidget::getID() const
  {
    return _id;
  }

  uint16_t IWidget::getHeight() const
  {
    return _height;
  }

  uint16_t IWidget::getWidth() const
  {
    return _width;
  }

  uint16_t IWidget::getBackColor() const
  {
    return _back_color;
  }

  uint16_t IWidget::getBorderColor() const
  {
    return _border_color;
  }

  bool IWidget::hasBorder() const
  {
    return _has_border;
  }

  void IWidget::setChangingBorder(bool state)
  {
    _need_change_border = state;
    _is_changed = true;
  }

  void IWidget::setChangingBack(bool state)
  {
    _need_change_back = state;
    _is_changed = true;
  }

  void IWidget::setFocusBorderColor(uint16_t color)
  {
    _focus_border_color = color;
    _is_changed = true;
  }

  uint16_t IWidget::getFocusBorderColor() const
  {
    return _focus_border_color;
  }

  void IWidget::setFocusBackColor(uint16_t color)
  {
    _focus_back_color = color;
    _is_changed = true;
  }

  uint16_t IWidget::getFocusBackColor() const
  {
    return _focus_back_color;
  }

  void IWidget::setFocus()
  {
    if (_has_focus)
      return;

    _is_changed = true;
    _has_focus = true;

    if (_need_change_border)
    {
      _old_border_state = _has_border;
      _has_border = true;

      _old_border_color = _border_color;
      _border_color = _focus_border_color;
    }

    if (_need_change_back)
    {
      _old_back_color = _back_color;
      _back_color = _focus_back_color;
    }
  }

  void IWidget::removeFocus()
  {
    if (!_has_focus)
      return;

    _is_changed = true;
    _has_focus = false;

    if (_need_change_border)
    {
      _need_clear_border = _has_border;
      _has_border = _old_border_state;
      _border_color = _old_border_color;
    }

    if (_need_change_back)
      _back_color = _old_back_color;
  }

  void IWidget::setVisibility(Visibility value)
  {
    _visibility = value;
    _is_changed = true;
  }

  IWidget::Visibility IWidget::getVisibility() const
  {
    return _visibility;
  }

  void IWidget::setTransparency(bool state)
  {
    _is_transparent = state;
  }

  bool IWidget::isTransparent() const
  {
    return _is_transparent;
  }

  IWidget* IWidget::findTouchableAt(uint16_t x, uint16_t y)
  {
    if (!_is_touchable || !hitTest(x, y))
      return nullptr;

    return this;
  }

  bool IWidget::hitTest(uint16_t x, uint16_t y) const
  {
    if (_parent)
      return (x > _parent->getXPos() + _x_pos && x < _parent->getXPos() + _x_pos + _width) &&
          (y > _parent->getYPos() + _y_pos && y < _parent->getYPos() + _y_pos + _height);
    else
      return (x > _x_pos && x < _x_pos + _width) && (y > _y_pos && y < _y_pos + _height);
  }

  void IWidget::setTouchable(bool state)
  {
    _is_touchable = state;
    _is_changed = true;
  }

  bool IWidget::isTouchable() const
  {
    return _is_touchable;
  }
}  // namespace pixeler