#pragma GCC optimize("O3")
#include "ComboItem.h"

namespace pixeler
{
  ComboItem::ComboItem(uint16_t widget_ID) : MenuItem(widget_ID, TYPE_COMBO_ITEM)
  {
  }

  ComboItem::~ComboItem()
  {
  }

  void ComboItem::copyTo(IWidget* widget) const
  {
    MenuItem::copyTo(widget);

    ComboItem* clone = static_cast<ComboItem*>(widget);
    clone->setLbl(_label->clone(_label->getID()));
    clone->_range = _range;
    clone->_selected_pos = _selected_pos;
    if (_img)
      clone->setImg(_img->clone(_img->getID()));
  }

  ComboItem* ComboItem::clone(uint16_t id) const
  {
    try
    {
      ComboItem* clone = new ComboItem(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void ComboItem::setRange(const std::vector<String>& range)
  {
    if (range.empty())
    {
      log_e("Вектор не може бути порожім");
      esp_restart();
    }

    _range = range;
    _selected_pos = 0;
    _label->setText(range[0]);
  }

  void ComboItem::selectPos(uint16_t pos)
  {
    if (_range.empty())
      return;

    if (pos >= _range.size())
      pos = _range.size() - 1;

    _selected_pos = pos;
    _label->setText(_range[_selected_pos]);
  }

  uint16_t ComboItem::getPos() const
  {
    return _selected_pos;
  }

  void ComboItem::scrollUp()
  {
    if (_range.empty())
      return;

    if (_selected_pos == 0)
      _selected_pos = _range.size() - 1;
    else
      --_selected_pos;

    _label->setText(_range[_selected_pos]);
  }

  void ComboItem::scrollDown()
  {
    if (_range.empty())
      return;

    if (_selected_pos < _range.size() - 1)
      ++_selected_pos;
    else
      _selected_pos = 0;

    _label->setText(_range[_selected_pos]);
  }
}  // namespace pixeler