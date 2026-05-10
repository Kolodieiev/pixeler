#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#include "Arduino_TFT_18bit.h"

#include "Arduino_DataBus.h"
#include "Arduino_GFX.h"

Arduino_TFT_18bit::Arduino_TFT_18bit(
    Arduino_DataBus* bus,
    int8_t rst,
    uint8_t r,
    bool ips,
    int16_t w,
    int16_t h,
    uint8_t col_offset1,
    uint8_t row_offset1,
    uint8_t col_offset2,
    uint8_t row_offset2)
    : Arduino_TFT(bus, rst, r, ips, w, h, col_offset1, row_offset1, col_offset2, row_offset2)
{
}

void Arduino_TFT_18bit::writeColor(uint16_t color)
{
  _bus->write((color & 0xF800) >> 8);
  _bus->write((color & 0x07E0) >> 3);
  _bus->write(color << 3);
}

void Arduino_TFT_18bit::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  writeAddrWindow(x, y, 1, 1);
  _bus->write((color & 0xF800) >> 8);
  _bus->write((color & 0x07E0) >> 3);
  _bus->write(color << 3);
}

void Arduino_TFT_18bit::writeRepeat(uint16_t color, uint32_t len)
{
#if defined(ESP8266) || defined(ESP32)
  uint8_t c[3] = {(uint8_t)((color & 0xF800) >> 8), (uint8_t)((color & 0x07E0) >> 3), (uint8_t)((color & 0x001F) << 3)};
  _bus->writePattern(c, 3, len);
#else
  uint8_t c1 = (uint8_t)((color & 0xF800) >> 8);
  uint8_t c2 = (uint8_t)((color & 0x07E0) >> 3);
  uint8_t c3 = (uint8_t)((color & 0x001F) << 3);
  while (len--)
  {
    _bus->write(c1);
    _bus->write(c2);
    _bus->write(c3);
  }
#endif
}

void Arduino_TFT_18bit::writePixels(uint16_t* data, uint32_t len)
{
  uint16_t d;
  while (len--)
  {
    d = *data++;
    _bus->write((d & 0xF800) >> 8);
    _bus->write((d & 0x07E0) >> 3);
    _bus->write(d << 3);
  }
}

// TFT tuned BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

void Arduino_TFT_18bit::draw16bitRGBBitmap(
    int16_t x,
    int16_t y,
    const uint16_t* bitmap,
    int16_t w,
    int16_t h)
{
  if (
      ((x + w - 1) < 0) ||  // Outside left
      ((y + h - 1) < 0) ||  // Outside top
      (x > _max_x) ||       // Outside right
      (y > _max_y)          // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                 // Clip left
      (y < 0) ||                 // Clip top
      ((x + w - 1) > _max_x) ||  // Clip right
      ((y + h - 1) > _max_y)     // Clip bottom
  )
  {
    Arduino_GFX::draw16bitRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint16_t d;
    startWrite();
    writeAddrWindow(x, y, w, h);
    for (int16_t j = 0; j < h; j++, y++)
    {
      for (int16_t i = 0; i < w; i++)
      {
        d = bitmap[j * w + i];
        _bus->write((d & 0xF800) >> 8);
        _bus->write((d & 0x07E0) >> 3);
        _bus->write(d << 3);
      }
    }
    endWrite();
  }
}
