#pragma once
#include "./DS3231ComnConst.h"
#include "./DS3231DateTime.h"
#include "bus/I2C_Bus.h"
#include "defines.h"

namespace pixeler
{
  class DS3231
  {
  public:
    bool begin();
    bool isDateTimeValid() const;
    bool isRunning() const;
    bool enable() const;
    bool disable() const;
    bool setDateTime(const DS3231DateTime& date_time) const;
    bool isConnected() const;
    DS3231DateTime getDateTime() const;
    uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day_of_month) const;
    DS3231DateTime compiledToDS3231DateTime(const char* date, const char* time) const;

  private:
    const uint8_t REG_TIMEDATE{0x00};
    const uint8_t REG_TIMEDATE_SIZE{7};
    const uint8_t CNTRL_BIT_EOSC{7};

    const uint8_t STS_BIT_BSY{2};
    const uint8_t STS_BIT_EN32KHZ{3};
    const uint8_t STS_BIT_OSF{7};
  };
}  // namespace pixeler
