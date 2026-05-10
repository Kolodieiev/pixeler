#pragma GCC optimize("O3")
#include "../Arduino_DataBus.h"

// #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4)  // Modify

#include "../Arduino_GFX.h"
#include "Arduino_RGB_Display.h"

Arduino_RGB_Display::Arduino_RGB_Display(
    int16_t w,
    int16_t h,
    Arduino_ESP32RGBPanel* rgbpanel,
    uint8_t r,
    Arduino_DataBus* bus,
    int8_t rst,
    const uint8_t* init_operations,
    size_t init_operations_len,
    uint8_t col_offset1,
    uint8_t row_offset1,
    uint8_t col_offset2,
    uint8_t row_offset2)
    : Arduino_GFX(w, h),
      _rgbpanel(rgbpanel),
      _bus(bus),
      _rst(rst),
      _init_operations(init_operations),
      _init_operations_len(init_operations_len),
      COL_OFFSET1(col_offset1),
      ROW_OFFSET1(row_offset1),
      COL_OFFSET2(col_offset2),
      ROW_OFFSET2(row_offset2)
{
  _fb_width = COL_OFFSET1 + WIDTH + COL_OFFSET2;
  _fb_height = ROW_OFFSET1 + HEIGHT + ROW_OFFSET2;
  _fb_max_x = _fb_width - 1;
  _fb_max_y = _fb_height - 1;
  _framebuffer_size = _fb_width * _fb_height * 2;
  MAX_X = WIDTH - 1;
  MAX_Y = HEIGHT - 1;
  setRotation(r);
}

bool Arduino_RGB_Display::begin(int32_t speed)
{
  if (speed != GFX_SKIP_DATABUS_BEGIN)
  {
    if (_bus)
    {
      if (!_bus->begin())
      {
        return false;
      }
    }
  }

  if (_rst != GFX_NOT_DEFINED)
  {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(100);
    digitalWrite(_rst, LOW);
    delay(120);
    digitalWrite(_rst, HIGH);
    delay(120);
  }
  else
  {
    if (_bus)
    {
      // Software Rest
      _bus->sendCommand(0x01);
      delay(120);
    }
  }

  if (_bus)
  {
    if (_init_operations_len > 0)
    {
      _bus->batchOperation((uint8_t*)_init_operations, _init_operations_len);
    }
  }

  _rgbpanel->begin(speed);
  _framebuffer = _rgbpanel->getFrameBuffer(_fb_width, _fb_height);

  if (!_framebuffer)
  {
    return false;
  }

  return true;
}

void Arduino_RGB_Display::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  int32_t y2 = y;
  switch (_rotation)
  {
    case 1:
      y2 = x;
      x = WIDTH - 1 - y;
      break;
    case 2:
      x = WIDTH - 1 - x;
      y2 = HEIGHT - 1 - y;
      break;
    case 3:
      y2 = HEIGHT - 1 - x;
      x = y;
      break;
  }

  x += COL_OFFSET1;
  y2 += ROW_OFFSET1;

  uint16_t* fb = _framebuffer + (y2 * _fb_width) + x;

  *fb = color;
}

void Arduino_RGB_Display::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  // log_i("writeFastVLine(x: %d, y: %d, h: %d)", x, y, h);
  switch (_rotation)
  {
    case 1:
      writeFastHLineCore(_height - y - h, x, h, color);
      break;
    case 2:
      writeFastVLineCore(_max_x - x, _height - y - h, h, color);
      break;
    case 3:
      writeFastHLineCore(y, _max_x - x, h, color);
      break;
    default:  // case 0:
      writeFastVLineCore(x, y, h, color);
  }
}

void Arduino_RGB_Display::writeFastVLineCore(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  // log_i("writeFastVLineCore(x: %d, y: %d, h: %d)", x, y, h);
  if (_ordered_in_range(x, 0, MAX_X) && h)
  {  // X on screen, nonzero height
    if (h < 0)
    {              // If negative height...
      y += h + 1;  //   Move Y to top edge
      h = -h;      //   Use positive height
    }
    if (y <= MAX_Y)
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
        if (y2 > MAX_Y)
        {
          h = MAX_Y - y + 1;
        }  // Clip bottom

        x += COL_OFFSET1;
        y += ROW_OFFSET1;
        uint16_t* fb = _framebuffer + ((int32_t)y * _fb_width) + x;

        while (h--)
        {
          *fb = color;
          fb += _fb_width;
        }
      }
    }
  }
}

void Arduino_RGB_Display::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  // log_i("writeFastHLine(x: %d, y: %d, w: %d)", x, y, w);
  switch (_rotation)
  {
    case 1:
      writeFastVLineCore(_max_y - y, x, w, color);
      break;
    case 2:
      writeFastHLineCore(_width - x - w, _max_y - y, w, color);
      break;
    case 3:
      writeFastVLineCore(y, _width - x - w, w, color);
      break;
    default:  // case 0:
      writeFastHLineCore(x, y, w, color);
  }
}

void Arduino_RGB_Display::writeFastHLineCore(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  // log_i("writeFastHLineCore(x: %d, y: %d, w: %d)", x, y, w);
  if (_ordered_in_range(y, 0, MAX_Y) && w)
  {  // Y on screen, nonzero width
    if (w < 0)
    {              // If negative width...
      x += w + 1;  //   Move X to left edge
      w = -w;      //   Use positive width
    }
    if (x <= MAX_X)
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
        if (x2 > MAX_X)
        {
          w = MAX_X - x + 1;
        }  // Clip right

        x += COL_OFFSET1;
        y += ROW_OFFSET1;
        uint16_t* fb = _framebuffer + ((int32_t)y * _fb_width) + x;
        int16_t writeSize = w * 2;
        while (w--)
        {
          *(fb++) = color;
        }
      }
    }
  }
}

void Arduino_RGB_Display::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  // log_i("writeFillRectPreclipped(x: %d, y: %d, w: %d, h: %d)", x, y, w, h);
  if (_rotation > 0)
  {
    int16_t t = x;
    switch (_rotation)
    {
      case 1:
        x = WIDTH - y - h;
        y = t;
        t = w;
        w = h;
        h = t;
        break;
      case 2:
        x = WIDTH - x - w;
        y = HEIGHT - y - h;
        break;
      case 3:
        x = y;
        y = HEIGHT - t - w;
        t = w;
        w = h;
        h = t;
        break;
    }
  }
  // log_i("adjusted writeFillRectPreclipped(x: %d, y: %d, w: %d, h: %d)", x, y, w, h);
  x += COL_OFFSET1;
  y += ROW_OFFSET1;
  uint16_t* row = _framebuffer;
  row += y * _fb_width;
  row += x;
  for (int j = 0; j < h; ++j)
  {
    for (int i = 0; i < w; ++i)
    {
      row[i] = color;
    }
    row += _fb_width;
  }
}

void Arduino_RGB_Display::draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h)
{
  x += COL_OFFSET1;
  y += ROW_OFFSET1;

  if (
      ((x + w - 1) < 0) ||  // Outside left
      ((y + h - 1) < 0) ||  // Outside top
      (x > MAX_X) ||        // Outside right
      (y > MAX_Y)           // Outside bottom
  )
    return;

  switch (_rotation)
  {
    case 1:
      drawBitmapToFramebufferRotate1(bitmap, w, h, _framebuffer, x, y, _fb_height, _fb_width);
      break;
    case 2:
      drawBitmapToFramebufferRotate2(bitmap, w, h, _framebuffer, x, y, _fb_width, _fb_height);
      break;
    case 3:
      drawBitmapToFramebufferRotate3(bitmap, w, h, _framebuffer, x, y, _fb_height, _fb_width);
      break;
    default:  // case 0:
      drawBitmapToFramebuffer(bitmap, w, h, _framebuffer, x, y, _fb_width, _fb_height);
  }
}

void Arduino_RGB_Display::flushMainBuff()
{
  Cache_WriteBack_Addr((uint32_t)_framebuffer, _framebuffer_size);
}

uint16_t* Arduino_RGB_Display::getFramebuffer()
{
  return _framebuffer;
}

#endif  // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
