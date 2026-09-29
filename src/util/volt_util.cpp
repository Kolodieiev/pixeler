#include "volt_util.h"

namespace pixeler
{
  float readPinVoltage(uint8_t pin, uint16_t samples_num, float r_div_k)
  {
    if (r_div_k <= 0.0f)
    {
      log_e("Некоректний коефіцієнт подільника напруги: %f", r_div_k);
      esp_restart();
    }

    uint32_t bat_mv = 0;

    for (uint8_t i = 0; i < samples_num; ++i)
      bat_mv += analogReadMilliVolts(pin);

    float avg_mv = (float)bat_mv / samples_num;
    return (avg_mv / 1000.0f) / r_div_k;
  }
}  // namespace pixeler
