#pragma GCC optimize("O3")
#include "Arduino_Canvas.h"

#include <hal/cache_hal.h>
#include <hal/cache_ll.h>

#include "../Arduino_DataBus.h"
#include "../Arduino_GFX.h"
#include "graphics_config.h"

Arduino_Canvas::Arduino_Canvas(
    int16_t w,
    int16_t h,
    Arduino_GFX* output,
    int16_t output_x,
    int16_t output_y,
    uint8_t r)
    : Arduino_GFX(w, h), _output(output), _output_x(output_x), _output_y(output_y), MAX_X{static_cast<uint16_t>(WIDTH - 1)}, MAX_Y{static_cast<uint16_t>(HEIGHT - 1)}
{
  setRotation(r);
}

Arduino_Canvas::~Arduino_Canvas()
{
  free(_framebuffer2);
}

bool Arduino_Canvas::begin(int32_t speed)
{
  if (!_output->begin(speed))
  {
    log_e("Помилка ініціалізації драйвера дисплея");
    esp_restart();
  }

  free(_framebuffer);
  _framebuffer = nullptr;

  uint32_t buf_align = cache_hal_get_cache_line_size(CACHE_LL_LEVEL_EXT_MEM, CACHE_TYPE_DATA);

  if (psramInit())
    _framebuffer = static_cast<uint16_t*>(heap_caps_aligned_alloc(buf_align, FRAMEBUFF_SIZE, MALLOC_CAP_SPIRAM));
  else
    _framebuffer = static_cast<uint16_t*>(aligned_alloc(buf_align, FRAMEBUFF_SIZE));

  if (!_framebuffer)
  {
    log_e("Memory allocation failed");
    esp_restart();
  }

#ifdef DOUBLE_BUFFERRING
  if (!psramInit())
  {
    log_e("Подвійна буферизація може працювати тільки на чіпах з PSRAM");
    esp_restart();
  }

  free(_framebuffer2);
  _framebuffer2 = nullptr;
  _framebuffer2 = static_cast<uint16_t*>(heap_caps_aligned_alloc(buf_align, FRAMEBUFF_SIZE, MALLOC_CAP_SPIRAM));

  if (!_framebuffer2)
  {
    log_e("Memory allocation failed");
    esp_restart();
  }

#endif  // DOUBLE_BUFFERRING

  log_e("Canvas ініціалізовано");
  return true;
}

void Arduino_Canvas::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  uint16_t* fb = _framebuffer;
  switch (_rotation)
  {
    case 1:
      fb += (int32_t)x * _height;
      fb += _max_y - y;
      *fb = color;
      break;
    case 2:
      fb += (int32_t)(_max_y - y) * _width;
      fb += _max_x - x;
      *fb = color;
      break;
    case 3:
      fb += (int32_t)(_max_x - x) * _height;
      fb += y;
      *fb = color;
      break;
    default:  // case 0:
      fb += (int32_t)y * _width;
      fb += x;
      *fb = color;
  }
}

void Arduino_Canvas::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
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

void Arduino_Canvas::writeFastVLineCore(int16_t x, int16_t y, int16_t h, uint16_t color)
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

        uint16_t* fb = _framebuffer + ((int32_t)y * WIDTH) + x;
        while (h--)
        {
          *fb = color;
          fb += WIDTH;
        }
      }
    }
  }
}

void Arduino_Canvas::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
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

void Arduino_Canvas::writeFastHLineCore(int16_t x, int16_t y, int16_t w, uint16_t color)
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

        uint16_t* fb = _framebuffer + ((int32_t)y * WIDTH) + x;
        while (w--)
        {
          *(fb++) = color;
        }
      }
    }
  }
}

void Arduino_Canvas::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
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

#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled)
  {
    ppaFill(x, y, w, h, color);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    uint16_t* row = _framebuffer;
    row += y * WIDTH;
    row += x;
    for (int j = 0; j < h; j++)
    {
      for (int i = 0; i < w; i++)
      {
        row[i] = color;
      }
      row += WIDTH;
    }
  }
}

void Arduino_Canvas::draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h)
{
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
      drawBitmapToFramebufferRotate1(bitmap, w, h, _framebuffer, x, y, _width, _height);
      break;
    case 2:
      drawBitmapToFramebufferRotate2(bitmap, w, h, _framebuffer, x, y, _width, _height);
      break;
    case 3:
      drawBitmapToFramebufferRotate3(bitmap, w, h, _framebuffer, x, y, _width, _height);
      break;
    default:  // case 0:
      drawBitmapToFramebuffer(bitmap, w, h, _framebuffer, x, y, _width, _height);
  }
}

void Arduino_Canvas::draw16bitRGBBitmapWithTranColor(int16_t x, int16_t y, const uint16_t* bitmap, uint16_t transparent_color, int16_t w, int16_t h)
{
  if (_rotation > 0)
  {
    Arduino_GFX::draw16bitRGBBitmapWithTranColor(x, y, bitmap, transparent_color, w, h);
  }
  else
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
    else
    {
      int16_t x_skip = 0;
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
        x_skip = (x + w - 1) - _max_x;
        w -= x_skip;
      }
      if (x < 0)
      {
        bitmap -= x;
        x_skip -= x;
        w += x;
        x = 0;
      }
      uint16_t* row = _framebuffer;
      row += y * _width;
      row += x;
      int16_t i;
      int16_t wi;
      uint16_t p;
      while (h--)
      {
        i = 0;
        wi = w;
        while (wi >= 4)
        {
          uint32_t b32 = *((uint32_t*)bitmap);
          p = (b32 & 0xffff);
          if (p != transparent_color)
          {
            row[i] = p;
          }
          ++i;
          p = (b32 & 0xffff0000) >> 16;
          if (p != transparent_color)
          {
            row[i] = p;
          }
          ++i;
          wi -= 2;
          bitmap += 2;
        }
        while (i < w)
        {
          p = *bitmap++;
          if (p != transparent_color)
          {
            row[i] = p;
          }
          ++i;
        }
        bitmap += x_skip;
        row += _width;
      }
    }
  }
}

void Arduino_Canvas::flushMainBuff()
{
  _output->draw16bitRGBBitmap(_output_x, _output_y, _framebuffer, WIDTH, HEIGHT);
}

void Arduino_Canvas::duplicateMainBuff()
{
#ifdef DOUBLE_BUFFERRING
  memcpy(_framebuffer2, _framebuffer, FRAMEBUFF_SIZE);
#else
  log_e("Подвійну буферизацію не було увімкнуто в налаштуваннях прошивки");
  esp_restart();
#endif  // DOUBLE_BUFFERRING
}

void Arduino_Canvas::flushSecondBuff()
{
#ifdef DOUBLE_BUFFERRING
  _output->draw16bitRGBBitmap(_output_x, _output_y, _framebuffer2, WIDTH, HEIGHT);
#else
  log_e("Подвійну буферизацію не було увімкнуто в налаштуваннях прошивки");
  esp_restart();
#endif  // DOUBLE_BUFFERRING
}

uint16_t* Arduino_Canvas::getFramebuffer()
{
  return _framebuffer;
}
