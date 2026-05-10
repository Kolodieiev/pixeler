#pragma GCC optimize("O3")

#include "IContext.h"

#include "../widget/layout/EmptyLayout.h"

namespace pixeler
{
  void IContext::tick()
  {
    if (loop())
    {
      if ((millis() - _upd_time) > UI_UPDATE_DELAY)
      {
        _upd_time = millis();

        _input.__update();

        update();

#ifdef GRAPHICS_ENABLED
        if (_gui_enabled)
        {
          xSemaphoreTake(_layout_mutex, portMAX_DELAY);
          _layout->onDraw();

          if (_notification)
          {
            _notification->onDraw();
          }

          if (_toast_label)
          {
            if (millis() - _toast_birthtime > _toast_lifetime)
              removeToast();
            else
              _toast_label->drawForced();
          }
          xSemaphoreGive(_layout_mutex);

#ifndef DIRECT_DRAWING
          _display.__flush();
#endif  // #ifndef DIRECT_DRAWING
        }
#endif  // GRAPHICS_ENABLED
      }
    }
  }

  ContextID IContext::getNextContextID() const
  {
    return _next_context_ID;
  }

  bool IContext::isReleased() const
  {
    return _is_released;
  }

  void IContext::openContextByID(ContextID context_ID)
  {
    _input.reset();
    _next_context_ID = context_ID;
    _is_released = true;
  }

  void IContext::release()
  {
    openContextByID(static_cast<ContextID>(0));
  }

#ifndef GRAPHICS_ENABLED
  IContext::IContext() {}
  IContext::~IContext() {}
#else  // GRAPHICS_ENABLED

  IContext::IContext() : _layout_mutex{xSemaphoreCreateMutex()},
                         _layout{new EmptyLayout(1)}
  {
    _layout->setBackColor(COLOR_YELLOW);
    _layout->setWidth(UI_WIDTH);
    _layout->setHeight(UI_HEIGHT);
  }

  IContext::~IContext()
  {
    delete _layout;
    delete _toast_label;

    vSemaphoreDelete(_layout_mutex);
  }

  void IContext::setLayout(IWidgetContainer* layout)
  {
    if (!layout)
    {
      log_e("Спроба встановити NULL-layout.");
      esp_restart();
    }

    if (_layout == layout)
      return;

    xSemaphoreTake(_layout_mutex, portMAX_DELAY);

    delete _layout;
    _layout = layout;

    xSemaphoreGive(_layout_mutex);
  }

  IWidgetContainer* IContext::getLayout() const
  {
    return _layout;
  }

  void IContext::showToast(const char* msg_txt, unsigned long duration)
  {
    if (!msg_txt)
      return;

    if (duration < 500 || duration > 5000)
      duration = 500;

    _toast_birthtime = millis();
    _toast_lifetime = duration;

    if (_toast_label)
    {
      _toast_label->setText(msg_txt);
      _toast_label->setAutoscroll(true);
      return;
    }

    _toast_label = new Label(1);
    _toast_label->setText(msg_txt);
    _toast_label->setBackColor(COLOR_WHITE);
    _toast_label->setTextColor(COLOR_BLACK);
    _toast_label->setAlign(IWidget::ALIGN_CENTER);
    _toast_label->setGravity(IWidget::GRAVITY_CENTER);
    _toast_label->setAutoscroll(true);
    _toast_label->setCornerRadius(10);
    _toast_label->setBorder(true);
    _toast_label->setBorderColor(COLOR_ORANGE);
    _toast_label->setHeight(25);
    _toast_label->setHPadding(4);

    if (UI_WIDTH < 120)
      _toast_label->setWidth(UI_WIDTH - 6);
    else
      _toast_label->setWidth(120);

    _toast_label->setPos(getCenterX(_toast_label), UI_HEIGHT - _toast_label->getHeight() - 15);
  }

  uint16_t IContext::getCenterX(const IWidget* widget) const
  {
    return widget ? (UI_WIDTH - widget->getWidth()) / 2 : 0;
  }

  uint16_t IContext::getCenterY(const IWidget* widget) const
  {
    return widget ? (UI_HEIGHT - widget->getHeight()) / 2 : 0;
  }

  void IContext::showNotification(Notification* notification)
  {
    _notification = notification;
  }

  void IContext::hideNotification()
  {
    _notification = nullptr;

    if (_layout)
      _layout->drawForced();
  }

  bool IContext::takeLayoutMutex()
  {
    return xSemaphoreTake(_layout_mutex, portMAX_DELAY);
  }

  void IContext::giveLayoutMutex()
  {
    xSemaphoreGive(_layout_mutex);
  }

  void IContext::removeToast()
  {
    delete _toast_label;
    _toast_label = nullptr;

    if (_layout)
      _layout->drawForced();
  }

#endif  // #ifdef GRAPHICS_ENABLED
}  // namespace pixeler