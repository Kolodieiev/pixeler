#pragma GCC optimize("O3")
#include "EmptyLayout.h"

namespace pixeler
{
  EmptyLayout::EmptyLayout(uint16_t widget_ID, TypeID type_ID) : IWidgetContainer(widget_ID, type_ID)
  {
  }

  EmptyLayout::~EmptyLayout() {}

  void EmptyLayout::onDraw()
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

      for (size_t i{0}; i < _widgets.size(); ++i)
        _widgets[i]->drawForced();
    }
  }

  void EmptyLayout::copyTo(IWidget* widget) const
  {
    IWidgetContainer::copyTo(widget);
    // Не має власних полів
  }

  EmptyLayout* EmptyLayout::clone(uint16_t id) const
  {
    try
    {
      EmptyLayout* clone = new EmptyLayout(id);
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
