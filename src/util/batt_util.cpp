#include "batt_util.h"

namespace pixeler
{
#ifdef HAS_BATTERY

  float readBattVoltage()
  {
    uint32_t bat_mv = 0;

    for (uint8_t i = 0; i < VOLTAGE_SAMP_NUM; ++i)
      bat_mv += analogReadMilliVolts(PIN_VOLT_MEASH);

    float avg_mv = (float)bat_mv / VOLTAGE_SAMP_NUM;
    return (avg_mv / 1000.0f) / R_DIV_K;
  }

#endif  // HAS_BATTERY
}  // namespace pixeler
