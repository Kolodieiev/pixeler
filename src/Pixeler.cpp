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
    setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);

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
        ContextID next_context_id = context->getNextContextID();
        delete context;

        const auto it = _context_id_map.find(next_context_id);
        if (it == _context_id_map.end())
        {
          log_e("Невідомий ідентифікатор контексту: %u", next_context_id);
          esp_restart();
        }

        context = it->second();
      }

      if (millis() - ts > WDT_GUARD_TIME)
      {
        delay(1);
        ts = millis();
      }
    }
  }
}  // namespace pixeler
