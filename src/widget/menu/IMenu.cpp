#pragma GCC optimize("O3")
#include "IMenu.h"

namespace pixeler
{
  IMenu::IMenu(uint16_t widget_ID, TypeID type_ID) : IWidgetContainer(widget_ID, type_ID) {}

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

  void IMenu::delWidgets()
  {
    IWidgetContainer::delWidgets();
    _first_item_index = 0;
    _cur_focus_pos = 0;
  }

  bool IMenu::delWidgetByID(uint16_t widget_ID)
  {
    if (!IWidgetContainer::delWidgetByID(widget_ID))
      return false;

    if (_widgets.empty())
    {
      _first_item_index = 0;
      _cur_focus_pos = 0;
      return true;
    }

    if (_first_item_index >= _widgets.size())
    {
      const uint16_t items_per_page = getItemsPerPage();
      if (_widgets.size() <= items_per_page)
        _first_item_index = 0;
      else
        _first_item_index = _widgets.size() - items_per_page;
    }

    if (!_has_touch_support && _cur_focus_pos >= _widgets.size())
    {
      _cur_focus_pos = _widgets.size() - 1;
      _widgets[_cur_focus_pos]->setFocus();
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
      _first_item_index = 0;

      if (!_has_touch_support)
        _widgets[_cur_focus_pos]->setFocus();
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
    _is_changed = true;
  }

  uint16_t IMenu::getItemsSpacing() const
  {
    return _items_spacing;
  }

  void IMenu::addItem(MenuItem* item)
  {
    addWidget(item);
  }

  uint16_t IMenu::getItemsPerPage() const
  {
    if (_orientation == VERTICAL)
      return _height / (_item_height + _items_spacing);
    else
      return _width / (_item_width + _items_spacing);
  }

  void IMenu::onDraw()
  {
    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
      {
        const uint16_t cycles_count = getCyclesCount();
        const uint16_t end_index = _first_item_index + cycles_count;
        for (uint16_t i{_first_item_index}; i < end_index; ++i)
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

    const size_t total = _widgets.size();
    if (total == 0)
    {
      clear();
      return;
    }

    if (_first_item_index >= total)
    {
      const uint16_t items_per_page = getItemsPerPage();
      if (total <= items_per_page)
        _first_item_index = 0;
      else
        _first_item_index = total - items_per_page;
    }

    if (!_has_touch_support)
    {
      if (_cur_focus_pos >= total)
        _cur_focus_pos = total - 1;

      _widgets[_cur_focus_pos]->setFocus();
    }

    drawItems(_first_item_index, getCyclesCount());
  }

  void IMenu::drawItems(uint16_t start, uint16_t count)
  {
    clear();

    if (_widgets.empty())
      return;

    uint16_t item_x_pos = 2;
    uint16_t item_y_pos = 2;
    const size_t container_size = _widgets.size();
    const uint16_t end_index = start + count;

    if (_orientation == VERTICAL)
    {
      const uint16_t item_w = _width - 4;
      const uint16_t y_step = _item_height + _items_spacing;

      for (uint16_t i{start}; i < end_index && i < container_size; ++i)
      {
        _widgets[i]->setPos(item_x_pos, item_y_pos);
        _widgets[i]->setHeight(_item_height);
        _widgets[i]->setWidth(item_w);
        _widgets[i]->onDraw();
        item_y_pos += y_step;
      }
    }
    else
    {
      const uint16_t item_h = _height - 4;
      const uint16_t x_step = _item_width + _items_spacing;

      for (uint16_t i{start}; i < end_index && i < container_size; ++i)
      {
        _widgets[i]->setPos(item_x_pos, item_y_pos);
        _widgets[i]->setHeight(item_h);
        _widgets[i]->setWidth(_item_width);
        _widgets[i]->onDraw();
        item_x_pos += x_step;
      }
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
    if (_first_item_index >= _widgets.size())
      return 0;

    uint16_t cycles_count = getItemsPerPage();
    const uint16_t items_to_end_count = _widgets.size() - _first_item_index;

    if (cycles_count > items_to_end_count)
      cycles_count = items_to_end_count;

    return cycles_count;
  }
}  // namespace pixeler
