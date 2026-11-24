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
    clone->_range = _range;
    clone->_selected_pos = _selected_pos;
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
      log_e("Вектор не може бути порожнім");
      esp_restart();
    }

    _range = range;
    _selected_pos = 0;
    _label->setText(range[0]);
  }

  void ComboItem::selectPos(uint16_t pos)
  {
    const size_t total = _range.size();
    if (total == 0)
      return;

    if (pos >= total)
      pos = total - 1;

    _selected_pos = pos;
    _label->setText(_range[_selected_pos]);
  }

  uint16_t ComboItem::getPos() const
  {
    return _selected_pos;
  }

  void ComboItem::scrollUp()
  {
    const size_t total = _range.size();
    if (total < 2)
      return;

    if (_selected_pos == 0)
      _selected_pos = total - 1;
    else
      --_selected_pos;

    _label->setText(_range[_selected_pos]);
  }

  void ComboItem::scrollDown()
  {
    const size_t total = _range.size();
    if (total < 2)
      return;

    if (_selected_pos < total - 1)
      ++_selected_pos;
    else
      _selected_pos = 0;

    _label->setText(_range[_selected_pos]);
  }
}  // namespace pixeler
