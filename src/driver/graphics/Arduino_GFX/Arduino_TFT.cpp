#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#include "Arduino_TFT.h"

#include "Arduino_DataBus.h"
#include "Arduino_GFX.h"
#include "font/glcdfont.h"

Arduino_TFT::Arduino_TFT(
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
    : Arduino_GFX(w, h), _bus(bus), _rst(rst), _ips(ips), COL_OFFSET1(col_offset1), ROW_OFFSET1(row_offset1), COL_OFFSET2(col_offset2), ROW_OFFSET2(row_offset2)
{
  _rotation = r;
}

bool Arduino_TFT::begin(int32_t speed)
{
  if (speed != GFX_SKIP_DATABUS_BEGIN)
  {
    if (_override_datamode != GFX_NOT_DEFINED)
    {
      if (!_bus->begin(speed, _override_datamode))
      {
        return false;
      }
    }
    else
    {
      if (!_bus->begin(speed))
      {
        return false;
      }
    }
  }

  tftInit();
  setRotation(_rotation);  // apply the setting rotation to the display
  setAddrWindow(0, 0, _width, _height);

  return true;
}

void Arduino_TFT::startWrite()
{
  _bus->beginWrite();
}

void Arduino_TFT::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  writeAddrWindow(x, y, 1, 1);
  _bus->write16(color);
}

void Arduino_TFT::writeRepeat(uint16_t color, uint32_t len)
{
  _bus->writeRepeat(color, len);
}

void Arduino_TFT::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  if (_ordered_in_range(x, 0, _max_x) && h)
  {  // X on screen, nonzero height
    if (h < 0)
    {              // If negative height...
      y += h + 1;  //   Move Y to top edge
      h = -h;      //   Use positive height
    }
    if (y <= _max_y)
    {  // Not off bottom
      int16_t y2 = y + h - 1;
      if (y2 >= 0)
      {  // Not off top
        // Line partly or fully overlaps screen
        if (y < 0)
        {
          y = 0;
          h = y2 + 1;
        }  // Clip top
        if (y2 > _max_y)
        {
          h = _max_y - y + 1;
        }  // Clip bottom
        writeFillRectPreclipped(x, y, 1, h, color);
      }
    }
  }
}

void Arduino_TFT::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  if (_ordered_in_range(y, 0, _max_y) && w)
  {  // Y on screen, nonzero width
    if (w < 0)
    {              // If negative width...
      x += w + 1;  //   Move X to left edge
      w = -w;      //   Use positive width
    }
    if (x <= _max_x)
    {  // Not off right
      int16_t x2 = x + w - 1;
      if (x2 >= 0)
      {  // Not off left
        // Line partly or fully overlaps screen
        if (x < 0)
        {
          x = 0;
          w = x2 + 1;
        }  // Clip left
        if (x2 > _max_x)
        {
          w = _max_x - x + 1;
        }  // Clip right
        writeFillRectPreclipped(x, y, w, 1, color);
      }
    }
  }
}

void Arduino_TFT::writeFillRectPreclipped(
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    uint16_t color)
{
#ifdef ESP8266
  yield();
#endif
  writeAddrWindow(x, y, w, h);
  writeRepeat(color, (uint32_t)w * h);
}

void Arduino_TFT::endWrite()
{
  _bus->endWrite();
}

void Arduino_TFT::setAddrWindow(int16_t x0, int16_t y0, uint16_t w, uint16_t h)
{
  startWrite();

  writeAddrWindow(x0, y0, w, h);

  endWrite();
}

void Arduino_TFT::setRotation(uint8_t r)
{
  Arduino_GFX::setRotation(r);
  switch (_rotation)
  {
    case 1:
      _xStart = ROW_OFFSET1;
      _yStart = COL_OFFSET2;
      break;
    case 2:
      _xStart = COL_OFFSET2;
      _yStart = ROW_OFFSET2;
      break;
    case 3:
      _xStart = ROW_OFFSET2;
      _yStart = COL_OFFSET1;
      break;
    case 4:
      _xStart = COL_OFFSET2;
      _yStart = ROW_OFFSET1;
      break;
    case 5:
      _xStart = ROW_OFFSET2;
      _yStart = COL_OFFSET2;
      break;
    case 6:
      _xStart = COL_OFFSET1;
      _yStart = ROW_OFFSET2;
      break;
    case 7:
      _xStart = ROW_OFFSET1;
      _yStart = COL_OFFSET1;
      break;
    default:  // case 0:
      _xStart = COL_OFFSET1;
      _yStart = ROW_OFFSET1;
      break;
  }
  _currentX = 0xFFFF;
  _currentY = 0xFFFF;
  _currentW = 0xFFFF;
  _currentH = 0xFFFF;
}

void Arduino_TFT::writeColor(uint16_t color)
{
  _bus->write16(color);
}

void Arduino_TFT::writeBytes(const uint8_t* data, uint32_t len)
{
  _bus->writeBytes(data, len);
}

void Arduino_TFT::writePixels(uint16_t* data, uint32_t len)
{
  _bus->writePixels(data, len);
}

void Arduino_TFT::pushColor(uint16_t color)
{
  _bus->beginWrite();
  writeColor(color);
  _bus->endWrite();
}

void Arduino_TFT::writeSlashLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t dx;
  int16_t dy;
  int16_t err;
  int16_t xs;
  int16_t step;
  int16_t len;

  bool steep = _diff(y1, y0) > _diff(x1, x0);
  if (steep)
  {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1)
  {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  dx = x1 - x0;
  dy = _diff(y1, y0);
  err = dx >> 1;
  xs = x0;
  step = (y0 < y1) ? 1 : -1;
  len = 0;

  while (x0 <= x1)
  {
    x0++;
    len++;
    err -= dy;
    if ((err < 0) || ((x0 > x1) && len))
    {
      if (steep)
      {
        writeFillRectPreclipped(y0, xs, 1, len, color);
      }
      else
      {
        writeFillRectPreclipped(xs, y0, len, 1, color);
      }
      err += dx;
      y0 += step;
      len = 0;
      xs = x0;
    }
  }
}

// TFT tuned BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

void Arduino_TFT::draw16bitRGBBitmap(
    int16_t x,
    int16_t y,
    const uint16_t* bitmap,
    int16_t w,
    int16_t h)
{
  if (
      ((y + h - 1) < 0) ||  // Outside top
      (y > _max_y)          // Outside bottom
  )
  {
    return;
  }
  else if (
      ((x + w - 1) < 0) ||  // Outside left
      (x > _max_x)          // Outside right
  )
  {
    return;
  }
  else
  {
    int16_t out_width = w;
    if ((y + h - 1) > _max_y)
    {
      h -= (y + h - 1) - _max_y;
    }
    if (y < 0)
    {
      bitmap -= y * w;
      h += y;
      y = 0;
    }
    if ((x + w - 1) > _max_x)
    {
      out_width -= (x + w - 1) - _max_x;
    }
    if (x < 0)
    {
      bitmap -= x;
      out_width += x;
      x = 0;
    }

    startWrite();
    writeAddrWindow(x, y, out_width, h);
    if (out_width < w)
    {
      for (int16_t j = 0; j < h; j++)
      {
        _bus->writePixels(bitmap, out_width);
        bitmap += w;
      }
    }
    else
    {
      _bus->writePixels(bitmap, (uint32_t)w * h);
    }
    endWrite();
  }
}

void Arduino_TFT::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg)
{
  uint16_t block_w;
  uint16_t block_h;

  if (u8g2Font)
  {
    Arduino_GFX::drawChar(x, y, c, color, bg);
  }
  else  // not u8g2Font
  {
    block_w = 6 * textsize_x;
    block_h = 8 * textsize_y;
    if (
        (x < _min_text_x) ||                  // Clip left
        (y < _min_text_y) ||                  // Clip top
        ((x + block_w - 1) > _max_text_x) ||  // Clip right
        ((y + block_h - 1) > _max_text_y)     // Clip bottom
    )
    {
      // partial draw char by parent class
      Arduino_GFX::drawChar(x, y, c, color, bg);
    }
    else
    {
      uint8_t col[5];
      for (int8_t i = 0; i < 5; i++)
      {
        col[i] = pgm_read_byte(&font[c * 5 + i]);
      }

      startWrite();
      if (bg != color)  // have background color
      {
        writeAddrWindow(x, y, block_w, block_h);

        uint16_t line_buf[block_w];
        if (textsize_x == 1)
        {
          line_buf[5] = bg;  // last column always bg
        }
        else
        {
          for (int8_t k = 0; k < textsize_x; k++)
          {
            line_buf[5 * textsize_x + k] = bg;
          }
        }
        uint8_t bit = 1;
        bool draw_dot;

        while (bit)
        {
          for (int8_t i = 0; i < 5; i++)
          {
            draw_dot = col[i] & bit;
            if (textsize_x == 1)
            {
              line_buf[i] = (draw_dot) ? color : bg;
            }
            else
            {
              if (draw_dot)
              {
                for (int8_t k = 0; k < textsize_x; k++)
                {
                  line_buf[i * textsize_x + k] = (k < (textsize_x - text_pixel_margin)) ? color : bg;
                }
              }
              else
              {
                for (int8_t k = 0; k < textsize_x; k++)
                {
                  line_buf[i * textsize_x + k] = bg;
                }
              }
            }
          }
          if (textsize_y == 1)
          {
            writePixels(line_buf, block_w);
          }
          else
          {
            for (int8_t l = 0; l < textsize_y; l++)
            {
              if (l < (textsize_y - text_pixel_margin))
              {
                writePixels(line_buf, block_w);
              }
              else
              {
                writeRepeat(bg, block_w);
              }
            }
          }
          bit <<= 1;
        }
      }
      else  // (bg == color), no background color
      {
        for (int8_t i = 0; i < 5; i++)
        {  // Char bitmap = 5 columns
          uint8_t line = col[i];
          for (int8_t j = 0; j < 8; j++, line >>= 1)
          {
            if (line & 1)
            {
              if (textsize_x == 1 && textsize_y == 1)
              {
                writePixelPreclipped(x + i, y + j, color);
              }
              else
              {
                writeFillRectPreclipped(x + i * textsize_x, y + j * textsize_y, textsize_x - text_pixel_margin, textsize_y - text_pixel_margin, color);
              }
            }
          }
        }
      }
      endWrite();
    }
  }
}
