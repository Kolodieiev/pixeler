#include "Pixeler.h"

#include "context/IContext.h"
#include "graphics_config.h"
#include "ui_config.h"

namespace pixeler
{
  void Pixeler::begin(uint32_t stack_depth_kb)
  {
    xTaskCreatePinnedToCore(pixelerContextTask, "mct", stack_depth_kb * 512, NULL, 10, NULL, 1);
  }

  void Pixeler::pixelerContextTask(void* params)
  {
    _input.__init();

#ifdef GRAPHICS_ENABLED
    _display.__init();
#endif

    IContext* context = new START_CONTEXT();

    unsigned long ts = millis();
    while (1)
    {
      if (!context->isReleased())
      {
        context->tick();
      }
      else
      {
        IContext* next_context = context->takeNextContext();

        if (!next_context) [[unlikely]]
        {
          log_e("Наступний контекст першого рівня не може бути null");
          esp_restart();
        }

        delete context;
        context = next_context;
      }

      if (millis() - ts > WDT_GUARD_TIME)
      {
        delay(1);
        ts = millis();
      }
    }
  }
}  // namespace pixeler
