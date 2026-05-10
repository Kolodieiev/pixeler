#pragma GCC optimize("O3")

#include "DynamicMenu.h"

namespace pixeler
{
  DynamicMenu::DynamicMenu(uint16_t widget_ID) : IMenu(widget_ID, TYPE_DYN_MENU) {}

  bool DynamicMenu::pageUp()
  {
    if (_widgets.empty())
      return false;

    if (_prev_items_load_handler)
    {
      std::vector<MenuItem*> temp_vec;
      _prev_items_load_handler(temp_vec, getItemsPerPage(), _widgets[_cur_focus_pos]->getID(), _prev_items_load_arg);

      if (!temp_vec.empty())
      {
        delWidgets();

        _widgets.reserve(temp_vec.size());
        for (size_t i{0}; i < temp_vec.size(); ++i)
        {
          temp_vec[i]->setParent(this);
          _widgets.push_back(temp_vec[i]);
        }

        drawItems(0, getCyclesCount());

        if (!_has_touch_support && _cur_focus_pos != _first_item_index)
        {
          _cur_focus_pos = _widgets.size() - 1;
          _widgets[_cur_focus_pos]->setFocus();
        }

        return true;
      }
    }
    return false;
  }

  bool DynamicMenu::pageDown()
  {
    if (_widgets.empty())
      return false;

    if (_next_items_load_handler)
    {
      std::vector<MenuItem*> temp_vec;
      _next_items_load_handler(temp_vec, getItemsPerPage(), _widgets[_widgets.size() - 1]->getID(), _next_items_load_arg);

      if (!temp_vec.empty())
      {
        delWidgets();

        _widgets.reserve(temp_vec.size());
        for (size_t i{0}; i < temp_vec.size(); ++i)
        {
          temp_vec[i]->setParent(this);
          _widgets.push_back(temp_vec[i]);
        }

        drawItems(0, getCyclesCount());

        if (!_has_touch_support && _cur_focus_pos != _first_item_index)
        {
          _cur_focus_pos = 0;
          _widgets[_cur_focus_pos]->setFocus();
        }

        return true;
      }
    }

    return false;
  }

  bool DynamicMenu::focusUp()
  {
    if (_has_touch_support || _widgets.empty())
      return false;

    if (_cur_focus_pos > 0)
    {
      IWidget* item = _widgets[_cur_focus_pos];
      item->removeFocus();

      --_cur_focus_pos;

      uint16_t cycles_count = getCyclesCount();

      if (_cur_focus_pos < _first_item_index)
      {
        --_first_item_index;
        drawItems(_first_item_index, cycles_count);
      }

      item = _widgets[_cur_focus_pos];
      item->setFocus();

      return true;
    }
    else if (_prev_items_load_handler)
    {
      std::vector<MenuItem*> temp_vec;
      _prev_items_load_handler(temp_vec, getItemsPerPage(), _widgets[_cur_focus_pos]->getID(), _prev_items_load_arg);

      if (!temp_vec.empty())
      {
        delWidgets();

        _widgets.reserve(temp_vec.size());
        for (size_t i{0}; i < temp_vec.size(); ++i)
        {
          temp_vec[i]->setParent(this);
          _widgets.push_back(temp_vec[i]);
        }

        drawItems(_first_item_index, getCyclesCount());
        _cur_focus_pos = _widgets.size() - 1;

        _widgets[_cur_focus_pos]->setFocus();

        return true;
      }
    }
    return false;
  }

  bool DynamicMenu::focusDown()
  {
    if (_has_touch_support || _widgets.empty())
      return false;

    if (_cur_focus_pos < _widgets.size() - 1)
    {
      IWidget* item = _widgets[_cur_focus_pos];
      item->removeFocus();

      ++_cur_focus_pos;

      uint16_t cycles_count = getCyclesCount();

      if (_cur_focus_pos > _first_item_index + cycles_count - 1)
      {
        ++_first_item_index;

        drawItems(_first_item_index, cycles_count);
      }

      item = _widgets[_cur_focus_pos];
      item->setFocus();

      return true;
    }
    else if (_next_items_load_handler)
    {
      std::vector<MenuItem*> temp_vec;
      _next_items_load_handler(temp_vec, getItemsPerPage(), _widgets[_cur_focus_pos]->getID(), _next_items_load_arg);

      if (!temp_vec.empty())
      {
        delWidgets();

        _widgets.reserve(temp_vec.size());
        for (size_t i{0}; i < temp_vec.size(); ++i)
        {
          temp_vec[i]->setParent(this);
          _widgets.push_back(temp_vec[i]);
        }

        drawItems(_first_item_index, getCyclesCount());
        _cur_focus_pos = _first_item_index;

        _widgets[_cur_focus_pos]->setFocus();

        return true;
      }
    }

    return false;
  }

  void DynamicMenu::setOnNextItemsLoadHandler(NextItemsLoadHandler_t handler, void* arg)
  {
    _next_items_load_handler = handler;
    _next_items_load_arg = arg;
  }

  void DynamicMenu::setOnPrevItemsLoadHandler(PrevItemsLoadHandler_t handler, void* arg)
  {
    _prev_items_load_handler = handler;
    _prev_items_load_arg = arg;
  }

  void DynamicMenu::copyTo(IWidget* widget) const
  {
    IMenu::copyTo(widget);

    DynamicMenu* clone = static_cast<DynamicMenu*>(widget);
    clone->_next_items_load_handler = _next_items_load_handler;
    clone->_prev_items_load_handler = _prev_items_load_handler;
    clone->_next_items_load_arg = _next_items_load_arg;
    clone->_prev_items_load_arg = _prev_items_load_arg;
  }

  DynamicMenu* DynamicMenu::clone(uint16_t id) const
  {
    try
    {
      DynamicMenu* clone = new DynamicMenu(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }
}  // namespace pixeler
