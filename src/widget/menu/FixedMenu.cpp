#pragma GCC optimize("O3")
#include "FixedMenu.h"

namespace pixeler
{
  FixedMenu::FixedMenu(uint16_t widget_ID) : IMenu(widget_ID, TYPE_FIX_MENU) {}

  bool FixedMenu::pageUp()
  {
    uint16_t capacity = getItemsPerPage();

    if (capacity == 0 || _widgets.empty())
      return false;

    if (_first_item_index > 0)
    {
      if (_first_item_index >= capacity)  // Перевіряємо, чи ми можемо відняти цілу сторінку
      {
        _first_item_index -= capacity;
      }
      else
      {
        _first_item_index = 0;  // Якщо до початку менше, ніж ціла сторінка, стаємо в нуль
      }
    }
    else if (_is_loop_enbl)
    {
      uint16_t total = _widgets.size();  // Переходимо до останньої можливої сторінки

      if (total <= capacity)
      {
        return false;
      }

      _first_item_index = ((total - 1) / capacity) * capacity;  // Розраховуємо індекс для відображення залишку елементів
    }
    else
    {
      return false;
    }

    if (!_has_touch_support && _cur_focus_pos != _first_item_index)
    {
      _widgets[_cur_focus_pos]->removeFocus();
      _cur_focus_pos = _first_item_index;
      _widgets[_cur_focus_pos]->setFocus();
    }

    drawItems(_first_item_index, capacity);
    return true;
  }

  bool FixedMenu::pageDown()  // TODO
  {
    uint16_t capacity = getItemsPerPage();

    if (capacity == 0 || _widgets.empty())
      return false;

    uint16_t next_first_pos = _first_item_index + capacity;

    if (next_first_pos < _widgets.size())  // Якщо є хоча б одни елемент що не видно на поточній сторінці меню
    {
      _first_item_index = next_first_pos;
    }
    else if (_is_loop_enbl && _first_item_index > 0)  // Якщо меню може зациклюватися і ми не на першому елементі
    {
      _first_item_index = 0;
    }
    else
    {
      return false;
    }

    if (!_has_touch_support && _cur_focus_pos != _first_item_index)
    {
      _widgets[_cur_focus_pos]->removeFocus();
      _cur_focus_pos = _first_item_index;
      _widgets[_cur_focus_pos]->setFocus();
    }

    drawItems(_first_item_index, capacity);
    return true;
  }

  bool FixedMenu::focusUp()
  {
    if (_has_touch_support || _widgets.empty())
      return false;

    IWidget* item = _widgets[_cur_focus_pos];
    item->removeFocus();

    const uint16_t capacity = getItemsPerPage();
    bool need_redraw = false;

    if (_cur_focus_pos > 0)
    {
      --_cur_focus_pos;

      if (_cur_focus_pos < _first_item_index)
      {
        --_first_item_index;
        need_redraw = true;
      }
    }
    else if (_is_loop_enbl)
    {
      const uint16_t total = _widgets.size();
      if (total > capacity)
      {
        need_redraw = true;
        _first_item_index = ((total - 1) / capacity) * capacity;
      }
      else
      {
        _first_item_index = 0;
      }

      _cur_focus_pos = total - 1;
    }

    item = _widgets[_cur_focus_pos];
    item->setFocus();

    if (need_redraw)
      drawItems(_first_item_index, getCyclesCount());

    return true;
  }

  bool FixedMenu::focusDown()
  {
    if (_has_touch_support || _widgets.empty())
      return false;

    IWidget* item = _widgets[_cur_focus_pos];
    item->removeFocus();

    const uint16_t capacity = getItemsPerPage();
    bool need_redraw = false;

    if (_cur_focus_pos < _widgets.size() - 1)
    {
      ++_cur_focus_pos;

      if (_cur_focus_pos >= _first_item_index + getCyclesCount())
      {
        need_redraw = true;
        ++_first_item_index;
      }
    }
    else if (_is_loop_enbl)
    {
      _cur_focus_pos = 0;
      _first_item_index = 0;

      need_redraw = _widgets.size() > capacity;
    }

    item = _widgets[_cur_focus_pos];
    item->setFocus();

    if (need_redraw)
      drawItems(_first_item_index, getCyclesCount());

    return true;
  }

  void FixedMenu::setLooped(bool state)
  {
    _is_loop_enbl = state;
  }

  void FixedMenu::setCurrFocusPos(uint16_t focus_pos)
  {
    const size_t total = _widgets.size();
    if (_has_touch_support || total < 2 || _cur_focus_pos == focus_pos || focus_pos >= total)
      return;

    IWidget* item = _widgets[_cur_focus_pos];
    item->removeFocus();

    _cur_focus_pos = focus_pos;

    const uint16_t capacity = getItemsPerPage();
    uint16_t to_end = total - 1 - _cur_focus_pos;

    if (to_end >= capacity)
      _first_item_index = _cur_focus_pos;
    else if (total <= capacity)
      _first_item_index = 0;
    else
      _first_item_index = total - capacity;

    item = _widgets[_cur_focus_pos];
    item->setFocus();

    // Відмальовуємо фактичну кількість на сторінку
    drawItems(_first_item_index, getCyclesCount());
  }

  uint16_t FixedMenu::getPagesCount() const
  {
    uint16_t capacity = getItemsPerPage();
    if (capacity == 0 || _widgets.empty())
      return 0;

    return (_widgets.size() + capacity - 1) / capacity;
  }

  uint16_t FixedMenu::getPageNum() const
  {
    uint16_t capacity = getItemsPerPage();
    if (capacity == 0 || _widgets.empty())
      return 0;

    return _first_item_index / capacity;
  }

  void FixedMenu::setPageNum(uint16_t page_pos)
  {
    const uint16_t capacity = getItemsPerPage();
    if (capacity == 0 || _widgets.empty())
      return;

    const uint16_t max_pages = getPagesCount();
    if (page_pos >= max_pages)
      _first_item_index = (max_pages - 1) * capacity;
    else
      _first_item_index = page_pos * capacity;

    drawItems(_first_item_index, getCyclesCount());
  }

  void FixedMenu::copyTo(IWidget* widget) const
  {
    IMenu::copyTo(widget);

    FixedMenu* clone = static_cast<FixedMenu*>(widget);
    clone->_is_loop_enbl = _is_loop_enbl;
  }

  FixedMenu* FixedMenu::clone(uint16_t id) const
  {
    try
    {
      FixedMenu* clone = new FixedMenu(id);
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
