#pragma GCC optimize("O3")
#include "KeyboardRow.h"

#include "../text/Label.h"

namespace pixeler
{
  KeyboardRow::KeyboardRow(uint16_t widget_ID) : IWidgetContainer(widget_ID, TYPE_KB_ROW) {}

  void KeyboardRow::copyTo(IWidget* widget) const
  {
    IWidgetContainer::copyTo(widget);

    KeyboardRow* clone = static_cast<KeyboardRow*>(widget);
    clone->_cur_focus_pos = _cur_focus_pos;
    clone->_btn_height = _btn_height;
    clone->_btn_width = _btn_width;
  }

  KeyboardRow* KeyboardRow::clone(uint16_t id) const
  {
    try
    {
      KeyboardRow* clone = new KeyboardRow(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  uint16_t KeyboardRow::getCurrBtnID() const
  {
    uint16_t id = getFocusBtn()->getID();
    return id;
  }

  String KeyboardRow::getCurrBtnTxt() const
  {
    const Label* lbl = getFocusBtn()->castTo<Label>();
    return lbl->getText();
  }

  bool KeyboardRow::focusUp()
  {
    if (_cur_focus_pos > 0)
    {
      IWidget* btn = getFocusBtn();
      btn->removeFocus();
      --_cur_focus_pos;
      btn = getFocusBtn();
      btn->setFocus();

      return true;
    }

    return false;
  }

  bool KeyboardRow::focusDown()
  {
    if (!_widgets.empty() && _cur_focus_pos < _widgets.size() - 1)
    {
      IWidget* btn = getFocusBtn();
      btn->removeFocus();
      ++_cur_focus_pos;
      btn = getFocusBtn();
      btn->setFocus();

      return true;
    }

    return false;
  }

  void KeyboardRow::setBtnHeight(uint16_t height)
  {
    _btn_height = height > 0 ? height : 1;
    _is_changed = true;
  }

  uint16_t KeyboardRow::getBtnsHeight() const
  {
    return _btn_height;
  }

  void KeyboardRow::setBtnWidth(uint16_t width)
  {
    _btn_width = width > 0 ? width : 1;
    _is_changed = true;
  }

  uint16_t KeyboardRow::getBtnsWidth() const
  {
    return _btn_width;
  }

  uint16_t KeyboardRow::getCurFocusPos() const
  {
    return _cur_focus_pos;
  }

  void KeyboardRow::setFocus(uint16_t pos)
  {
    if (_widgets.empty())
      return;

    IWidget* btn = getFocusBtn();
    btn->removeFocus();

    if (pos > _widgets.size() - 1)
      _cur_focus_pos = _widgets.size() - 1;
    else
      _cur_focus_pos = pos;

    btn = getFocusBtn();
    btn->setFocus();
  }

  void KeyboardRow::removeFocus()
  {
    getFocusBtn()->removeFocus();
    _cur_focus_pos = 0;
  }

  IWidget* KeyboardRow::getFocusBtn() const
  {
    if (_widgets.empty())
    {
      log_e("Не додано жодної кнопки");
      esp_restart();
    }

    IWidget* item = _widgets[_cur_focus_pos];

    if (!item)
    {
      log_e("Кнопку не знайдено");
      esp_restart();
    }

    return item;
  }

  void KeyboardRow::onDraw()
  {
    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
        for (size_t i{0}; i < _widgets.size(); ++i)
          _widgets[i]->onDraw();

      return;
    }

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    clear();

    const size_t count = _widgets.size();
    if (count == 0)
      return;

    const uint16_t y = (_height > _btn_height) ? (_height - _btn_height) / 2 : 2;

    const uint32_t total_btn_width = _btn_width * count;
    const uint32_t total_free_space = (_width > total_btn_width) ? (_width - total_btn_width) : 0;
    const uint16_t num_gaps = count + 1;

    const uint16_t step = total_free_space / num_gaps;
    const uint16_t extra_pixels = total_free_space % num_gaps;

    uint16_t current_extra = extra_pixels;
    uint16_t x = step;

    if (current_extra >= num_gaps)
    {
      x++;
      current_extra -= num_gaps;
    }

    for (size_t i = 0; i < count; ++i)
    {
      _widgets[i]->setPos(x, y);
      _widgets[i]->setWidth(_btn_width);
      _widgets[i]->setHeight(_btn_height);
      _widgets[i]->onDraw();

      x += _btn_width + step;

      // Додаємо піксель до наступного проміжку
      current_extra += extra_pixels;
      if (current_extra >= num_gaps)
      {
        ++x;
        current_extra -= num_gaps;
      }
    }
  }
}  // namespace pixeler
