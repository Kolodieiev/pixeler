#pragma GCC optimize("O3")
#include "Keyboard.h"

namespace pixeler
{
  Keyboard::Keyboard(uint16_t widget_ID) : IWidgetContainer(widget_ID, TYPE_KEYBOARD) {}

  void Keyboard::copyTo(IWidget* widget) const
  {
    IWidgetContainer::copyTo(widget);

    Keyboard* clone = static_cast<Keyboard*>(widget);
    clone->_cur_focus_row_pos = _cur_focus_row_pos;

    // Інші параметри лишаємо по замовченню, щоб не зламати малювання
    // clone->_first_drawing = _first_drawing;
    // clone->_has_manual_settings = _has_manual_settings;
  }

  Keyboard* Keyboard::clone(uint16_t id) const
  {
    try
    {
      Keyboard* clone = new Keyboard(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  uint16_t Keyboard::getCurrBtnID() const
  {
    return getFocusRow()->getCurrBtnID();
  }

  String Keyboard::getCurrBtnTxt() const
  {
    return getFocusRow()->getCurrBtnTxt();
  }

  void Keyboard::focusUp()
  {
    const size_t total = _widgets.size();
    if (total < 2)
      return;

    KeyboardRow* row = getFocusRow();
    const uint16_t focus_pos = row->getCurFocusPos();

    row->removeFocus();

    if (_cur_focus_row_pos > 0)
      --_cur_focus_row_pos;
    else
      _cur_focus_row_pos = total - 1;

    getFocusRow()->setFocus(focus_pos);
  }

  void Keyboard::focusDown()
  {
    const size_t total = _widgets.size();
    if (total < 2)
      return;

    KeyboardRow* row = getFocusRow();
    const uint16_t focus_pos = row->getCurFocusPos();

    row->removeFocus();

    if (_cur_focus_row_pos < total - 1)
      ++_cur_focus_row_pos;
    else
      _cur_focus_row_pos = 0;

    getFocusRow()->setFocus(focus_pos);
  }

  void Keyboard::focusLeft()
  {
    if (_widgets.empty())
      return;

    KeyboardRow* row = getFocusRow();
    if (row->focusUp())
      return;

    const size_t row_size = row->getSize();
    if (row_size > 1)
      row->setFocus(row_size - 1);
  }

  void Keyboard::focusRight()
  {
    if (_widgets.empty())
      return;

    KeyboardRow* row = getFocusRow();
    if (row->focusDown())
      return;

    if (row->getSize() > 1)
      row->setFocus(0);
  }

  void Keyboard::setFocusPos(uint16_t x, uint16_t y)
  {
    const size_t total = _widgets.size();
    if (total == 0)
      return;

    _has_manual_settings = true;

    getFocusRow()->removeFocus();

    _cur_focus_row_pos = (y >= total) ? (total - 1) : y;

    getFocusRow()->setFocus(x);
  }

  KeyboardRow* Keyboard::getFocusRow() const
  {
    const size_t total = _widgets.size();
    if (total == 0)
    {
      log_e("Не додано жодної KeyboardRow");
      esp_restart();
    }

    if (_cur_focus_row_pos >= total)
    {
      log_e("Некоректна позиція рядка фокусу");
      esp_restart();
    }

    KeyboardRow* row = _widgets[_cur_focus_row_pos]->castTo<KeyboardRow>();
    if (!row)
    {
      log_e("KeyboardRow не знайдено");
      esp_restart();
    }

    return row;
  }

  void Keyboard::onDraw()
  {
    const size_t total = _widgets.size();

    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
      {
        for (size_t i{0}; i < total; ++i)
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

    clear();

    if (total == 0)
      return;

    if (_first_drawing && !_has_manual_settings)
    {
      _first_drawing = false;
      getFocusRow()->setFocus(0);
    }

    const uint16_t x{2};
    const uint16_t row_width = (_width > 4) ? (_width - 4) : 0;
    uint16_t y{2};

    for (size_t i{0}; i < total; ++i)
    {
      _widgets[i]->setPos(x, y);
      _widgets[i]->setWidth(row_width);
      _widgets[i]->onDraw();
      y += _widgets[i]->getHeight();
    }
  }
}  // namespace pixeler
