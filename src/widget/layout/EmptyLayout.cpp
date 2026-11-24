#include "EmptyLayout.h"

namespace pixeler
{
  EmptyLayout::EmptyLayout(uint16_t widget_ID, TypeID type_ID) : IWidgetContainer(widget_ID, type_ID)
  {
  }

  void EmptyLayout::onDraw()
  {
    if (!_is_changed)
    {
      if (_visibility != INVISIBLE && _is_enabled)
      {
        for (IWidget* widget : _widgets)
          widget->onDraw();
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

    for (IWidget* widget : _widgets)
      widget->drawForced();
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
