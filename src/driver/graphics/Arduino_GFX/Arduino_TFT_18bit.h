#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#ifndef _ARDUINO_TFT_18BIT_H_
#define _ARDUINO_TFT_18BIT_H_

#include "Arduino_DataBus.h"
#include "Arduino_GFX.h"
#include "Arduino_TFT.h"

class Arduino_TFT_18bit : public Arduino_TFT
{
public:
  Arduino_TFT_18bit(Arduino_DataBus* bus, int8_t rst, uint8_t r, bool ips, int16_t w, int16_t h, uint8_t col_offset1, uint8_t row_offset1, uint8_t col_offset2, uint8_t row_offset2);

  void writeColor(uint16_t color) override;
  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override;
  void writeRepeat(uint16_t color, uint32_t len) override;
  void writePixels(uint16_t* data, uint32_t len) override;
  void draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) override;

protected:
private:
};

#endif
