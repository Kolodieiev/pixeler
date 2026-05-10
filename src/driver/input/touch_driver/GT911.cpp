#pragma GCC optimize("O3")

#include "GT911.h"

#ifdef GT911_DRIVER

#define GT911_ADDR1 0x5Du
#define GT911_CONFIG_SIZE 0xB9u

// Основні регістри GT911
#define GT911_CONFIG_START 0x8047
#define GT911_X_MAX_L 0x8048
#define GT911_Y_MAX_L 0x804A
#define GT911_CONFIG_CHKSUM 0x80FFu
#define GT911_CONFIG_FRESH 0x8100u
#define GT911_POINT_INFO 0x814Eu
#define GT911_POINT_1 0x814Fu

namespace pixeler
{
  GT911::GT911()
  {
    _chip_addr = GT911_ADDR1;
  }

  void GT911::resetChip()
  {
    if (PIN_TOUCH_INT < 0 && PIN_TOUCH_RST < 0)
      return;

    pinMode(PIN_TOUCH_INT, OUTPUT);
    pinMode(PIN_TOUCH_RST, OUTPUT);

    // Послідовність вибору I2C адреси
    digitalWrite(PIN_TOUCH_INT, 0);
    digitalWrite(PIN_TOUCH_RST, 0);
    delay(10);
    digitalWrite(PIN_TOUCH_INT, 0);
    delay(1);
    digitalWrite(PIN_TOUCH_RST, 1);
    delay(5);
    digitalWrite(PIN_TOUCH_INT, 0);
    delay(50);
    pinMode(PIN_TOUCH_INT, INPUT);
    delay(50);
  }

  bool GT911::__begin()
  {
    log_e("GT911 Init:");

    if (_i2c.begin() && _i2c.hasConnect(_chip_addr))
    {
      _i2c.setBufferSize(GT911_CONFIG_SIZE);
      resetChip();
      uint8_t _conf_buf[GT911_CONFIG_SIZE]{0};
      _i2c.readRegister16(_chip_addr, GT911_CONFIG_START, _conf_buf, GT911_CONFIG_SIZE);
      setResolution(_conf_buf, _width, _height);
      log_e("Successful");
      return true;
    }
    log_e("Fail");
    return false;
  }

  void GT911::__update()
  {
    if (_is_locked)
    {
      if (millis() - _lock_time < _lock_duration)
        return;

      _is_locked = false;
    }

    // bool int_active = (digitalRead(PIN_TOUCH_INT) == HIGH);
    // if (!int_active && !_is_holded)
    //   return;

    uint8_t point_info = 0;
    if (!_i2c.readRegister16(_chip_addr, GT911_POINT_INFO, &point_info, 1))
      return;

    bool buffer_status = (point_info >> 7) & 1;
    uint8_t points = point_info & 0x0F;

    if (buffer_status && points > 0)
    {
      uint8_t data[7];
      if (!_i2c.readRegister16(_chip_addr, GT911_POINT_1, data, 7))
        return;

      if (data[0] == 0x0A && data[1] == 0x0A)
        return;

      uint16_t x = data[1] | (data[2] << 8);
      uint16_t y = data[3] | (data[4] << 8);
      setTouchPos(x, y);
      _is_holded = true;
    }
    else
    {
      _is_holded = false;
    }

    processTouch();

    // Обов'язкове скидання статусу для отримання наступного кадру
    uint8_t reset_val = 0;
    _i2c.writeRegister16(_chip_addr, GT911_POINT_INFO, &reset_val, sizeof(reset_val));
  }

  void GT911::setResolution(uint8_t* out_conf_buf, uint16_t width, uint16_t height)
  {
    uint8_t* ptr = &out_conf_buf[GT911_X_MAX_L - GT911_CONFIG_START];

    ptr[0] = width & 0xFF;          // X Low
    ptr[1] = (width >> 8) & 0xFF;   // X High
    ptr[2] = height & 0xFF;         // Y Low
    ptr[3] = (height >> 8) & 0xFF;  // Y High

    reflashConfig(out_conf_buf);
  }

  void GT911::calculateChecksum(uint8_t* out_conf_buf)
  {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < GT911_CONFIG_SIZE; ++i)
      checksum += out_conf_buf[i];

    // Формула контрольної суми за специфікацією Goodix
    out_conf_buf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START] = (~checksum) + 1;
  }

  void GT911::reflashConfig(uint8_t* out_conf_buf)
  {
    calculateChecksum(out_conf_buf);
    if (!_i2c.writeRegister16(_chip_addr, GT911_CONFIG_CHKSUM, &out_conf_buf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START], 1))
      return;
    uint8_t reflash_val = 1;
    _i2c.writeRegister16(_chip_addr, GT911_CONFIG_FRESH, &reflash_val, sizeof(reflash_val));
  }

}  // namespace pixeler

#endif  // GT911_DRIVER
