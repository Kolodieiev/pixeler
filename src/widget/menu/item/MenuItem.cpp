#pragma GCC optimize("O3")
#include "MenuItem.h"

namespace pixeler
{
  MenuItem::MenuItem(uint16_t widget_ID, TypeID type_ID) : IWidget(widget_ID, type_ID)
  {
    setLbl(new Label(1));
  }

  MenuItem::~MenuItem()
  {
    delete _label;
    delete _img;
  }

  String MenuItem::getText() const
  {
    return _label->getText();
  }

  Label* MenuItem::getLbl() const
  {
    return _label;
  }

  void MenuItem::setText(const String& text)
  {
    _label->setText(text);
  }

  void MenuItem::onDraw()
  {
    if (!_is_changed)
    {
      if (_img)
        _img->onDraw();
      _label->onDraw();
      return;
    }

    _is_changed = false;

    clear();

    uint8_t img_width{0};
    if (_img)
    {
      img_width = _img->getWidth() + ITEM_PADDING;
      _img->setPos(ITEM_PADDING, (_height - _img->getHeight()) * 0.5);
      _img->setBackColor(_back_color);
      _img->onDraw();
    }

    _label->setHeight(_height - 2);
    _label->setPos(img_width + ITEM_PADDING, 1);
    _label->setWidth(_width - ITEM_PADDING * 2 - img_width);

    if (_has_focus && _need_change_back)
      _label->setFocus();
    else
      _label->removeFocus();

    _label->onDraw();
  }

  void MenuItem::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    MenuItem* clone = static_cast<MenuItem*>(widget);
    clone->setLbl(_label->clone(_label->getID()));
    if (_img)
      clone->setImg(_img->clone(_img->getID()));
  }

  MenuItem* MenuItem::clone(uint16_t id) const
  {
    try
    {
      MenuItem* clone = new MenuItem(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void MenuItem::setImg(Image* img_ptr)
  {
    if (img_ptr == _img)
      return;

    delete _img;
    _img = img_ptr;

    if (_img)
      _img->setParent(this);

    _is_changed = true;
  }

  Image* MenuItem::getImg() const
  {
    return _img;
  }

  void MenuItem::setLbl(Label* lbl_ptr)
  {
    if (!lbl_ptr)
    {
      log_e("Label не може бути null");
      esp_restart();
    }

    if (lbl_ptr == _label)
      return;

    delete _label;
    _label = lbl_ptr;

    _is_changed = true;

    _label->setParent(this);
    _label->setBorder(false);
    _label->setChangingBorder(false);
    _label->setGravity(GRAVITY_CENTER);

    _label->setBackColor(_back_color);
    _label->setFocusBackColor(_focus_back_color);
    _label->setChangingBack(true);
    _label->setTransparency(_is_transparent);
  }
}  // namespace pixeler
