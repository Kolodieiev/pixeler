#pragma GCC optimize("O3")
#include "AXS15231B.h"

#ifdef AXS15231B_DRIVER

#define AXS15231B_ADDR 0x3B
#define I2C_BUFF_SIZE 128

#define AXS_MAX_TOUCH_NUMBER 1
#define AXS_BUF_SIZE (AXS_MAX_TOUCH_NUMBER * 6 + 2)

// 11-байтна команда з закодованим розміром відповіді
static const uint8_t READ_CMD[11] = {
    0xB5, 0xAB, 0xA5, 0x5A,
    0x00, 0x00,
    (uint8_t)(AXS_BUF_SIZE >> 8),
    (uint8_t)(AXS_BUF_SIZE & 0xFF),
    0x00, 0x00, 0x00};

namespace pixeler
{
  void AXS15231B::resetChip()
  {
    if (PIN_TOUCH_INT < 0 && PIN_TOUCH_RST < 0)
      return;

    pinMode(PIN_TOUCH_RST, OUTPUT);
    pinMode(PIN_TOUCH_INT, INPUT);

    digitalWrite(PIN_TOUCH_RST, 1);
    delay(50);
    digitalWrite(PIN_TOUCH_RST, 0);
    delay(50);
    digitalWrite(PIN_TOUCH_RST, 1);
    delay(50);
  }

  bool AXS15231B::__begin()
  {
    log_e("AXS15231B Init:");

    if (_i2c.begin() && _i2c.hasConnect(AXS15231B_ADDR))
    {
      _i2c.setBufferSize(I2C_BUFF_SIZE);
      resetChip();
      log_e("Successful");
      return true;
    }
    log_e("Fail");
    return false;
  }

  void AXS15231B::__update()
  {
    if (_is_locked)
    {
      if (millis() - _lock_time < _lock_duration)
        return;

      _is_locked = false;
    }

    // bool int_active = (digitalRead(PIN_TOUCH_INT) == LOW);
    // if (!int_active && !_is_holded)
    //   return;

    if (!_i2c.write(AXS15231B_ADDR, READ_CMD, sizeof(READ_CMD)))
      return;

    uint8_t data[AXS_BUF_SIZE]{0};
    if (!_i2c.read(AXS15231B_ADDR, data, (uint8_t)AXS_BUF_SIZE))
      return;

    if (data[0] == 0x0A && data[1] == 0x0A)
      return;

    // data[0] - зарезервовано
    // data[1] - кількість активних точок
    // data[2] = [event_flag(7:6)][rsvd(5:4)][X_high(3:0)]
    // data[3] = X_low
    // data[4] = [touch_id(7:4)][Y_high(3:0)]
    // data[5] = Y_low
    uint8_t point_num = data[1] & 0x0F;
    uint8_t event_flag = (data[2] >> 6) & 0x03;  // 0=down, 1=up, 2=contact

    if (point_num == 1 && event_flag != 1)
    {
      uint16_t x = ((data[2] & 0x0F) << 8) | data[3];
      uint16_t y = ((data[4] & 0x0F) << 8) | data[5];
      setTouchPos(x, y);
      _is_holded = true;
    }
    else
    {
      _is_holded = false;
    }

    processTouch();
  }
}  // namespace pixeler

#endif  // AXS15231B_DRIVER
