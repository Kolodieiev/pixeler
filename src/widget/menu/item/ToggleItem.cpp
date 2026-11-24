#include "ToggleItem.h"

namespace pixeler
{
  ToggleItem::ToggleItem(uint16_t widget_ID) : MenuItem(widget_ID, TYPE_TOGGLE_ITEM)
  {
    setToggle(new ToggleSwitch(1));
  }

  ToggleItem::~ToggleItem()
  {
    delete _toggle;
  }

  void ToggleItem::onDraw()
  {
    if (!_is_changed)
    {
      if (_image)
        _image->onDraw();
      _label->onDraw();
      _toggle->onDraw();
      return;
    }

    _is_changed = false;

    clear();

    uint16_t current_x = ITEM_PADDING;

    if (_image)
    {
      _image->setPos(current_x, (_height - _image->getHeight()) / 2);
      _image->setBackColor(_back_color);
      _image->onDraw();
      current_x += _image->getWidth() + ITEM_PADDING;
    }

    const uint16_t toggle_x = _width - ITEM_PADDING - _toggle->getWidth();
    _toggle->setPos(toggle_x, (_height - _toggle->getHeight()) / 2);
    _toggle->onDraw();

    _label->setHeight(_height - 2);
    _label->setPos(current_x, 1);
    _label->setWidth(toggle_x - current_x - ITEM_PADDING);

    if (_has_focus && _need_change_back)
      _label->setFocus();
    else
      _label->removeFocus();

    _label->onDraw();
  }

  void ToggleItem::copyTo(IWidget* widget) const
  {
    MenuItem::copyTo(widget);

    ToggleItem* clone = static_cast<ToggleItem*>(widget);
    clone->setToggle(_toggle->clone(_toggle->getID()));
  }

  ToggleItem* ToggleItem::clone(uint16_t id) const
  {
    try
    {
      ToggleItem* clone = new ToggleItem(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void ToggleItem::setToggle(ToggleSwitch* toggle_switch)
  {
    if (!toggle_switch)
    {
      log_e("ToggleSwitch не може бути null");
      esp_restart();
    }

    if (toggle_switch == _toggle)
      return;

    delete _toggle;
    _toggle = toggle_switch;

    _is_changed = true;

    _toggle->setParent(this);
    _toggle->setChangingBorder(false);
  }

  ToggleSwitch* ToggleItem::getToggle() const
  {
    return _toggle;
  }

  void ToggleItem::setOn(bool state)
  {
    _toggle->setOn(state);
  }

  void ToggleItem::toggle()
  {
    _toggle->toggle();
  }

  bool ToggleItem::isOn() const
  {
    return _toggle->isOn();
  }
}  // namespace pixeler
