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
    uint16_t id = getFocusRow()->getCurrBtnID();
    return id;
  }

  String Keyboard::getCurrBtnTxt() const
  {
    const KeyboardRow* row = getFocusRow();
    return row->getCurrBtnTxt();
  }

  void Keyboard::focusUp()
  {
    KeyboardRow* row = getFocusRow();

    uint16_t focusPos = row->getCurFocusPos();

    row->removeFocus();

    if (_cur_focus_row_pos > 0)
      _cur_focus_row_pos--;
    else
      _cur_focus_row_pos = _widgets.size() - 1;

    row = getFocusRow();
    row->setFocus(focusPos);
  }

  void Keyboard::focusDown()
  {
    KeyboardRow* row = getFocusRow();
    uint16_t focusPos = row->getCurFocusPos();
    row->removeFocus();

    if (_cur_focus_row_pos < _widgets.size() - 1)
      ++_cur_focus_row_pos;
    else
      _cur_focus_row_pos = 0;

    row = getFocusRow();
    row->setFocus(focusPos);
  }

  void Keyboard::focusLeft()
  {
    KeyboardRow* row = getFocusRow();

    if (!row->focusUp())
    {
      row->removeFocus();
      row->setFocus(row->getSize() - 1);
    }
  }

  void Keyboard::focusRight()
  {
    KeyboardRow* row = getFocusRow();

    if (!row->focusDown())
    {
      row->removeFocus();
      row->setFocus(0);
    }
  }

  uint16_t Keyboard::getFocusXPos() const
  {
    return getFocusRow()->getCurFocusPos();
  }

  uint16_t Keyboard::getFocusYPos() const
  {
    return _cur_focus_row_pos;
  }

  void Keyboard::setFocusPos(uint16_t x, uint16_t y)
  {
    if (_widgets.empty())
      return;

    _has_manual_settings = true;

    KeyboardRow* row = getFocusRow();
    row->removeFocus();

    if (y > _widgets.size() - 1)
      _cur_focus_row_pos = _widgets.size() - 1;
    else
      _cur_focus_row_pos = y;

    row = getFocusRow();
    row->setFocus(x);
  }

  KeyboardRow* Keyboard::getFocusRow() const
  {
    if (_widgets.empty())
    {
      log_e("Не додано жодної KeyboardRow");
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
    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
        for (size_t i{0}; i < _widgets.size(); ++i)
          _widgets[i]->onDraw();
    }
    else
    {
      _is_changed = false;

      if (_visibility == INVISIBLE)
      {
        hide();
        return;
      }

      clear();

      if (_first_drawing && !_has_manual_settings)
      {
        _first_drawing = false;

        if (!_widgets.empty())
          getFocusRow()->setFocus(0);
      }

      uint16_t x{2};
      uint16_t y{2};

      for (size_t i{0}; i < _widgets.size(); ++i)
      {
        _widgets[i]->setPos(x, y);
        _widgets[i]->setWidth(_width - 4);
        _widgets[i]->onDraw();
        y += _widgets[i]->getHeight();
      }
    }
  }
}  // namespace pixeler
