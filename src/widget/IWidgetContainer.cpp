#pragma GCC optimize("O3")
#include "IWidgetContainer.h"

namespace pixeler
{
  IWidgetContainer::IWidgetContainer(uint16_t widget_ID, TypeID type_ID) : IWidget(widget_ID, type_ID, true) {}

  IWidgetContainer::~IWidgetContainer()
  {
    delWidgets();
  }

  void IWidgetContainer::addWidget(IWidget* widget_ptr)
  {
    if (!widget_ptr)
    {
      log_e("Контейнер: %u. *IWidget не може бути NULL.", getID());
      esp_restart();
    }

    uint16_t search_ID = widget_ptr->getID();

    if (search_ID == 0)
    {
      log_e("Контейнер: %u. WidgetID повинен бути > 0.", getID());
      esp_restart();
    }

    for (const IWidget* widget : _widgets)
    {
      if (widget->getID() == search_ID)
      {
        log_e("Контейнер: %u. WidgetID повинен бути унікальним. Повторюється з: %u.", getID(), widget->getID());
        esp_restart();
      }
    }

    widget_ptr->setParent(this);
    _widgets.push_back(widget_ptr);
    _is_changed = true;
  }

  bool IWidgetContainer::delWidgetByID(uint16_t widget_ID)
  {
    for (auto i_b = _widgets.begin(), i_e = _widgets.end(); i_b != i_e; ++i_b)
    {
      if ((*i_b)->getID() == widget_ID)
      {
        delete *i_b;
        _widgets.erase(i_b);
        _is_changed = true;

        return true;
      }
    }

    return false;
  }

  IWidget* IWidgetContainer::getWidgetByID(uint16_t widget_ID)
  {
    for (auto& widget_ptr : _widgets)
    {
      if (widget_ptr->getID() == widget_ID)
      {
        return widget_ptr;
      }
    }

    return nullptr;
  }

  IWidget* IWidgetContainer::getWidgetByIndx(uint16_t widget_indx)
  {
    IWidget* result{nullptr};

    if (_widgets.size() > widget_indx)
      result = _widgets[widget_indx];

    return result;
  }

  IWidget* IWidgetContainer::findTouchableAt(uint16_t x, uint16_t y)
  {
    if (!hitTest(x, y))
      return nullptr;

    for (auto i_b = _widgets.rbegin(), i_e = _widgets.rend(); i_b != i_e; ++i_b)
    {
      if (IWidget* found = (*i_b)->findTouchableAt(x, y))
        return found;
    }

    return _is_touchable ? this : nullptr;
  }

  void IWidgetContainer::delWidgets()
  {
    for (auto* widget_ptr : _widgets)
      delete widget_ptr;

    _widgets.clear();

    _is_changed = true;
  }

  uint16_t IWidgetContainer::getSize() const
  {
    return _widgets.size();
  }

  void IWidgetContainer::enable()
  {
    _is_enabled = true;
  }

  void IWidgetContainer::disable()
  {
    _is_enabled = false;
  }

  bool IWidgetContainer::isEnabled() const
  {
    return _is_enabled;
  }

  void IWidgetContainer::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    IWidgetContainer* clone = static_cast<IWidgetContainer*>(widget);
    clone->_is_enabled = _is_enabled;

    for (const auto& widget_ptr : _widgets)
      clone->addWidget(widget_ptr->clone(widget_ptr->getID()));
  }
}  // namespace pixeler
