#pragma GCC optimize("O3")
#include "IMenu.h"

namespace pixeler
{
  IMenu::IMenu(uint16_t widget_ID, TypeID type_ID) : IWidgetContainer(widget_ID, type_ID) {}

  void IMenu::delWidgets()
  {
    IWidgetContainer::delWidgets();
    _first_item_index = 0;
    _cur_focus_pos = 0;
  }

  bool IMenu::delWidgetByID(uint16_t widget_ID)
  {
    if (IWidgetContainer::delWidgetByID(widget_ID))
      return false;

    if (_widgets.empty())
    {
      _first_item_index = 0;
      _cur_focus_pos = 0;
    }
    else
    {
      if (_first_item_index >= _widgets.size())
      {
        if (getItemsPerPage() <= _widgets.size())
          _first_item_index = 0;
        else
          _first_item_index = _widgets.size() - getItemsPerPage();
      }

      if (!_has_touch_support && _cur_focus_pos >= _widgets.size())
      {
        _cur_focus_pos = _widgets.size() - 1;
        _widgets[_cur_focus_pos]->setFocus();
      }
    }

    return true;
  }

  void IMenu::setItemHeight(uint16_t height)
  {
    _item_height = height > 0 ? height : 1;
    _is_changed = true;
  }

  uint16_t IMenu::getItemHeight() const
  {
    return _orientation == VERTICAL ? _item_height : _height - 4;
  }

  void IMenu::setItemWidth(uint16_t width)
  {
    _item_width = width > 0 ? width : 1;
    _is_changed = true;
  }

  uint16_t IMenu::getItemWidth() const
  {
    return _orientation == HORIZONTAL ? _item_width : _width - 4;
  }

  void IMenu::setOrientation(const Orientation orientation)
  {
    _orientation = orientation;
    _is_changed = true;
  }

  IWidget::Orientation IMenu::getOrientation() const
  {
    return _orientation;
  }

  void IMenu::setTouchSupport(bool state)
  {
    _has_touch_support = state;

    if (!_widgets.empty())
    {
      _widgets[_cur_focus_pos]->removeFocus();
      _cur_focus_pos = 0;
      _widgets[_cur_focus_pos]->setFocus();
      _first_item_index = 0;
    }

    _is_changed = true;
  }

  uint16_t IMenu::getCurrItemID() const
  {
    if (_has_touch_support || _widgets.empty())
      return 0;

    return _widgets[_cur_focus_pos]->getID();
  }

  uint16_t IMenu::getCurrFocusPos() const
  {
    return _cur_focus_pos;
  }

  String IMenu::getCurrItemText() const
  {
    if (_has_touch_support || _widgets.empty())
      return emptyString;

    const MenuItem* item = static_cast<MenuItem*>(_widgets[_cur_focus_pos]);
    return item->getText();
  }

  IWidget* IMenu::getCurrItem()
  {
    if (_has_touch_support || _widgets.empty())
      return nullptr;

    return _widgets[_cur_focus_pos];
  }

  void IMenu::setItemsSpacing(uint16_t items_spacing)
  {
    _items_spacing = items_spacing;
  }

  uint16_t IMenu::getItemsSpacing() const
  {
    return _items_spacing;
  }

  void IMenu::addItem(MenuItem* item_ptr)
  {
    addWidget(item_ptr);
  }

  uint16_t IMenu::getItemsPerPage() const
  {
    return static_cast<float>(_height) / _item_height;
  }

  void IMenu::onDraw()
  {
    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
      {
        uint16_t cycles_count = getCyclesCount();
        for (uint16_t i{_first_item_index}; i < _first_item_index + cycles_count; ++i)
          _widgets[i]->onDraw();
      }

      return;
    }

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    if (_widgets.empty())
    {
      clear();
      return;
    }

    uint16_t cycles_count = getCyclesCount();

    if (_first_item_index >= _widgets.size())
      _first_item_index = _widgets.size() - 1;

    if (!_has_touch_support)
    {
      if (_cur_focus_pos >= _widgets.size())
        _cur_focus_pos = _widgets.size() - 1;

      IWidget* item = _widgets[_cur_focus_pos];
      item->setFocus();
    }

    drawItems(_first_item_index, cycles_count);
  }

  void IMenu::copyTo(IWidget* widget) const
  {
    IWidgetContainer::copyTo(widget);

    IMenu* clone = static_cast<IMenu*>(widget);
    clone->_first_item_index = _first_item_index;
    clone->_cur_focus_pos = _cur_focus_pos;
    clone->_item_height = _item_height;
    clone->_item_width = _item_width;
    clone->_items_spacing = _items_spacing;
    clone->_orientation = _orientation;
    clone->_has_touch_support = _has_touch_support;
  }

  void IMenu::drawItems(uint16_t start, uint16_t count)
  {
    clear();

    if (_widgets.empty())
      return;

    uint16_t itemXPos = 2;
    uint16_t itemYPos = 2;
    size_t container_size = _widgets.size();
    for (uint16_t i{start}; i < start + count && i < container_size; ++i)
    {
      _widgets[i]->setPos(itemXPos, itemYPos);

      if (_orientation == VERTICAL)
      {
        _widgets[i]->setHeight(_item_height);
        _widgets[i]->setWidth(_width - 4);
        itemYPos += (_item_height + _items_spacing);
      }
      else
      {
        _widgets[i]->setHeight(_height - 4);
        _widgets[i]->setWidth(_item_width);
        itemXPos += (_item_width + _items_spacing);
      }

      _widgets[i]->onDraw();
    }
  }

  IWidget* IMenu::findTouchableAt(uint16_t x, uint16_t y)
  {
    if (!hitTest(x, y))
      return nullptr;

    uint16_t last_index = getCyclesCount() + _first_item_index;

    for (int i = _first_item_index; i < last_index; ++i)
    {
      if (IWidget* found = _widgets[i]->findTouchableAt(x, y))
        return found;
    }

    return _is_touchable ? this : nullptr;
  }

  uint16_t IMenu::getCyclesCount() const
  {
    uint16_t cycles_count;

    if (_orientation == VERTICAL)
      cycles_count = static_cast<float>(_height) / (_item_height + _items_spacing);
    else
      cycles_count = static_cast<float>(_width) / (_item_width + _items_spacing);

    uint16_t items_to_end_count = _widgets.size() - _first_item_index;

    if (cycles_count > items_to_end_count)
      cycles_count = items_to_end_count;

    return cycles_count;
  }
}  // namespace pixeler
