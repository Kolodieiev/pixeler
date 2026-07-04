#pragma GCC optimize("O3")
#include "SpinItem.h"

namespace pixeler
{
  SpinItem::SpinItem(uint16_t widget_ID) : MenuItem(widget_ID, TYPE_SPIN_ITEM)
  {
    setSpin(new SpinBox(1));
  }

  SpinItem::~SpinItem()
  {
    delete _spinbox;
  }

  void SpinItem::onDraw()
  {
    if (!_is_changed)
    {
      if (_image)
        _image->onDraw();
      _label->onDraw();
      _spinbox->onDraw();
      return;
    }

    _is_changed = false;

    clear();

    uint8_t img_width{0};
    if (_image)
    {
      _image->setParent(this);
      img_width = _image->getWidth() + ITEM_PADDING;
      _image->setPos(ITEM_PADDING, (_height - _image->getHeight()) * 0.5);
      _image->setBackColor(_back_color);
      _image->onDraw();
    }

    _spinbox->setHeight(_spinbox->getCharHgt() + 4);
    _spinbox->setPos(_width - ITEM_PADDING - _spinbox->getWidth(), (_height - _spinbox->getHeight()) * 0.5);
    _spinbox->onDraw();

    _label->setHeight(_height - 4);
    _label->setPos(img_width + ITEM_PADDING, 1);
    _label->setWidth(_width - ITEM_PADDING * 2 - img_width - (_spinbox->getWidth() + ITEM_PADDING));

    if (_has_focus && _need_change_back)
      _label->setFocus();
    else
      _label->removeFocus();

    _label->onDraw();
  }

  void SpinItem::copyTo(IWidget* widget) const
  {
    MenuItem::copyTo(widget);

    SpinItem* clone = static_cast<SpinItem*>(widget);
    clone->setLbl(_label->clone(_label->getID()));
    clone->setSpin(_spinbox->clone(_spinbox->getID()));
    if (_image)
      clone->setImg(_image->clone(_image->getID()));
  }

  SpinItem* SpinItem::clone(uint16_t id) const
  {
    try
    {
      SpinItem* clone = new SpinItem(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void SpinItem::setSpin(SpinBox* spinbox)
  {
    if (!spinbox)
    {
      log_e("SpinBox не може бути null");
      esp_restart();
    }

    if (spinbox == _spinbox)
      return;

    delete _spinbox;
    _spinbox = spinbox;

    _is_changed = true;

    _spinbox->setParent(this);
    _spinbox->setChangingBorder(false);
  }

  SpinBox* SpinItem::getSpin() const
  {
    return _spinbox;
  }

  void SpinItem::up()
  {
    _spinbox->up();
  }

  void SpinItem::down()
  {
    _spinbox->down();
  }

  void SpinItem::setMin(float min)
  {
    _spinbox->setMin(min);
  }

  void SpinItem::setMax(float max)
  {
    _spinbox->setMax(max);
  }

  void SpinItem::setValue(float value)
  {
    _spinbox->setValue(value);
  }

  float SpinItem::getValue() const
  {
    return _spinbox->getValue();
  }
}  // namespace pixeler
