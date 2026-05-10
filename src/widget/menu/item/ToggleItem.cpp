#pragma GCC optimize("O3")
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
      if (_img)
        _img->onDraw();
      _label->onDraw();
      _toggle->onDraw();
      return;
    }

    _is_changed = false;

    clear();

    uint8_t img_width{0};
    if (_img)
    {
      _img->setParent(this);
      img_width = _img->getWidth() + ITEM_PADDING;
      _img->setPos(ITEM_PADDING, (_height - _img->getHeight()) * 0.5);
      _img->setBackColor(_back_color);
      _img->onDraw();
    }

    _toggle->setPos(_width - ITEM_PADDING - _toggle->getWidth(), (_height - _toggle->getHeight()) * 0.5);
    _toggle->onDraw();

    _label->setHeight(_height - 2);
    _label->setPos(img_width + ITEM_PADDING, 1);
    _label->setWidth(_width - ITEM_PADDING * 2 - img_width - (_toggle->getWidth() + ITEM_PADDING));

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
    clone->setLbl(_label->clone(_label->getID()));
    clone->setToggle(_toggle->clone(_toggle->getID()));
    if (_img)
      clone->setImg(_img->clone(_img->getID()));
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

  void ToggleItem::setToggle(ToggleSwitch* togg_switch_ptr)
  {
    if (!togg_switch_ptr)
    {
      log_e("ToggleSwitch не може бути null");
      esp_restart();
    }

    if (togg_switch_ptr == _toggle)
      return;

    delete _toggle;
    _toggle = togg_switch_ptr;

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
