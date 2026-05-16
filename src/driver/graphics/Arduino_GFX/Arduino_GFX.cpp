#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 *
 * Arc function come from:
 * https://github.com/lovyan03/LovyanGFX.git
 */
#include "Arduino_GFX.h"

#include <pgmspace.h>

#include "Arduino_DataBus.h"
#include "float.h"
#include "font/glcdfont.h"
#include "graphics_config.h"

#if CONFIG_IDF_TARGET_ESP32P4 && defined(DIRECT_DRAWING)
#error "Пряме малювання не підтримуєтсья на ESP32P4. Використовуй канву!"
#endif  // #if CONFIG_IDF_TARGET_ESP32P4 && defined(DIRECT_DRAWING)

/**************************************************************************/
/*!
  @brief  Instatiate a GFX context for graphics! Can only be done by a superclass
  @param  w   Display width, in pixels
  @param  h   Display height, in pixels
*/
/**************************************************************************/
Arduino_GFX::Arduino_GFX(int16_t w, int16_t h) : FRAMEBUFF_SIZE{static_cast<uint32_t>(w * h * sizeof(uint16_t))},
                                                 WIDTH(w),
                                                 HEIGHT(h)
{
  _width = WIDTH;
  _height = HEIGHT;
  _max_x = _width - 1;   ///< x zero base bound
  _max_y = _height - 1;  ///< y zero base bound
  _min_text_x = 0;
  _min_text_y = 0;
  _max_text_x = _max_x;
  _max_text_y = _max_y;
  cursor_x = 0;
  cursor_y = 0;
  textcolor = 0xFFFF;
  textbgcolor = 0xFFFF;
  textsize_x = 1;
  textsize_y = 1;
  text_pixel_margin = 0;
  _rotation = 0;
  wrap = true;
  u8g2Font = NULL;

#if CONFIG_IDF_TARGET_ESP32P4
  initPPA();
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
}

#if CONFIG_IDF_TARGET_ESP32P4
void Arduino_GFX::initPPA()
{
  ppa_client_config_t ppa_config = {
      .oper_type = PPA_OPERATION_FILL,
      .max_pending_trans_num = 1,
      .data_burst_length = PPA_DATA_BURST_LENGTH_128};

  if (esp_err_t ret = ppa_register_client(&ppa_config, &_ppa_fill) != ESP_OK)
  {
    log_e("Помилка реєстрації FILL клієнта: %s", esp_err_to_name(ret));
    esp_restart();
  }

  ppa_client_config_t srm_config = {
      .oper_type = PPA_OPERATION_SRM,
      .max_pending_trans_num = 1,
      .data_burst_length = PPA_DATA_BURST_LENGTH_128};

  if (esp_err_t ret = ppa_register_client(&srm_config, &_ppa_srm) != ESP_OK)
  {
    log_e("Помилка реєстрації SRM клієнта: %s", esp_err_to_name(ret));
    esp_restart();
  }

  log_i("PPA ініціалізовано");

  _fill_oper = {
      .out = {
          .buffer = nullptr,
          .buffer_size = FRAMEBUFF_SIZE,
          .pic_w = 0,
          .pic_h = 0,
          .block_offset_x = 0,
          .block_offset_y = 0,
          .fill_cm = PPA_FILL_COLOR_MODE_RGB565,
      },
      .fill_block_w = 0,
      .fill_block_h = 0,
      .fill_color_val = 0,
      .mode = PPA_TRANS_MODE_BLOCKING,
  };

  _srm_oper = {
      .in = {
          .buffer = nullptr,
          .pic_w = 0,
          .pic_h = 0,
          .block_w = 0,
          .block_h = 0,
          .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
      },
      .out = {
          .buffer = nullptr,
          .buffer_size = FRAMEBUFF_SIZE,
          .pic_w = 0,
          .pic_h = 0,
          .block_offset_x = 0,
          .block_offset_y = 0,
          .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
      },
      .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
      .scale_x = 1.0f,
      .scale_y = 1.0f,
      .mode = PPA_TRANS_MODE_BLOCKING,
  };
}
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

Arduino_GFX::~Arduino_GFX()
{
  free(_framebuffer);
}

#if CONFIG_IDF_TARGET_ESP32P4
void Arduino_GFX::ppaFill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  _fill_oper.out.buffer = _framebuffer;
  _fill_oper.out.pic_w = WIDTH;
  _fill_oper.out.pic_h = HEIGHT;
  _fill_oper.out.block_offset_x = x;
  _fill_oper.out.block_offset_y = y;
  _fill_oper.fill_block_w = w;
  _fill_oper.fill_block_h = h;
  _fill_oper.fill_color_val = color16RGBTo24RGB(color);

  ppa_do_fill(_ppa_fill, &_fill_oper);
}

void Arduino_GFX::ppaRotate(const void* buff_from,
                            uint16_t buff_f_w,
                            uint16_t buff_f_h,
                            uint16_t x,
                            uint16_t y,
                            void* buff_to,
                            uint16_t buff_t_w,
                            uint16_t buff_t_h,
                            ppa_srm_rotation_angle_t angle)
{
  _srm_oper.in.buffer = buff_from;
  _srm_oper.in.pic_w = buff_f_w;
  _srm_oper.in.pic_h = buff_f_h;
  _srm_oper.in.block_w = buff_f_w;
  _srm_oper.in.block_h = buff_f_h;

  _srm_oper.out.buffer = buff_to;
  _srm_oper.out.buffer_size = FRAMEBUFF_SIZE;
  _srm_oper.out.pic_w = buff_t_w;
  _srm_oper.out.pic_h = buff_t_h;
  _srm_oper.out.block_offset_x = x;
  _srm_oper.out.block_offset_y = y;

  _srm_oper.rotation_angle = angle;

  ppa_do_scale_rotate_mirror(_ppa_srm, &_srm_oper);
}

uint32_t Arduino_GFX::color16RGBTo24RGB(uint16_t color565)
{
  uint32_t r = (color565 >> 11) & 0x1F;
  uint32_t g = (color565 >> 5) & 0x3F;
  uint32_t b = color565 & 0x1F;

  r = (r * 521) >> 6;
  g = (g * 1025) >> 8;
  b = (b * 521) >> 6;

  return (r << 16) | (g << 8) | b;
}

#endif  // #if CONFIG_IDF_TARGET_ESP32P4

/**************************************************************************/
/*!
  @brief  Write a line. Check straight or slash line and call corresponding function
  @param  x0      Start point x coordinate
  @param  y0      Start point y coordinate
  @param  x1      End point x coordinate
  @param  y1      End point y coordinate
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  if (x0 == x1)
  {
    if (y0 > y1)
    {
      _swap_int16_t(y0, y1);
    }
    writeFastVLine(x0, y0, y1 - y0 + 1, color);
  }
  else if (y0 == y1)
  {
    if (x0 > x1)
    {
      _swap_int16_t(x0, x1);
    }
    writeFastHLine(x0, y0, x1 - x0 + 1, color);
  }
  else
  {
    writeSlashLine(x0, y0, x1, y1, color);
  }
}

/**************************************************************************/
/*!
  @brief  Write a line.  Bresenham's algorithm - thx wikpedia
  @param  x0      Start point x coordinate
  @param  y0      Start point y coordinate
  @param  x1      End point x coordinate
  @param  y1      End point y coordinate
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::writeSlashLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
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

  int16_t dx = x1 - x0;
  int16_t dy = _diff(y1, y0);
  int16_t err = dx >> 1;
  int16_t step = (y0 < y1) ? 1 : -1;

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      writePixel(y0, x0, color);
    }
    else
    {
      writePixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0)
    {
      err += dx;
      y0 += step;
    }
  }
}

/**************************************************************************/
/*!
  @brief  Start a display-writing routine, overwrite in subclasses.
*/
/**************************************************************************/
GFX_INLINE void Arduino_GFX::startWrite()
{
}

void Arduino_GFX::writePixel(int16_t x, int16_t y, uint16_t color)
{
  if (_ordered_in_range(y, 0, _max_y) && _ordered_in_range(x, 0, _max_x))
    writePixelPreclipped(x, y, color);
}

/**************************************************************************/
/*!
  @brief  Write a pixel, overwrite in subclasses if startWrite is defined!
  @param  x       x coordinate
  @param  y       y coordinate
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  startWrite();
  writePixel(x, y, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Write a perfectly vertical line, overwrite in subclasses if startWrite is defined!
  @param  x       Top-most x coordinate
  @param  y       Top-most y coordinate
  @param  h       Height in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  for (int16_t i = y; i < y + h; i++)
  {
    writePixel(x, i, color);
  }
}

/**************************************************************************/
/*!
  @brief  Write a perfectly horizontal line, overwrite in subclasses if startWrite is defined!
  @param  x       Left-most x coordinate
  @param  y       Left-most y coordinate
  @param  w       Width in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  for (int16_t i = x; i < x + w; i++)
  {
    writePixel(i, y, color);
  }
}

/**************************************************************************/
/*!
  @brief  Draw a filled rectangle to the display. Not self-contained;
    should follow startWrite(). Typically used by higher-level
    graphics primitives; user code shouldn't need to call this and
    is likely to use the self-contained fillRect() instead.
    writeFillRect() performs its own edge clipping and rejection;
    see writeFillRectPreclipped() for a more 'raw' implementation.
  @param  x       Horizontal position of first corner.
  @param  y       Vertical position of first corner.
  @param  w       Rectangle width in pixels (positive = right of first
      corner, negative = left of first corner).
  @param  h       Rectangle height in pixels (positive = below first
      corner, negative = above first corner).
  @param  color   16-bit fill color in '565' RGB format.
  @note   Written in this deep-nested way because C by definition will
    optimize for the 'if' case, not the 'else' -- avoids branches
    and rejects clipped rectangles at the least-work possibility.
*/
/**************************************************************************/
void Arduino_GFX::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if (w && h)
  {  // Nonzero width and height?
    if (w < 0)
    {              // If negative width...
      x += w + 1;  //   Move X to left edge
      w = -w;      //   Use positive width
    }
    if (x <= _max_x)
    {  // Not off right
      if (h < 0)
      {              // If negative height...
        y += h + 1;  //   Move Y to top edge
        h = -h;      //   Use positive height
      }
      if (y <= _max_y)
      {  // Not off bottom
        int16_t x2 = x + w - 1;
        if (x2 >= 0)
        {  // Not off left
          int16_t y2 = y + h - 1;
          if (y2 >= 0)
          {  // Not off top
            // Rectangle partly or fully overlaps screen
            if (x < 0)
            {
              x = 0;
              w = x2 + 1;
            }  // Clip left
            if (y < 0)
            {
              y = 0;
              h = y2 + 1;
            }  // Clip top
            if (x2 > _max_x)
            {
              w = _max_x - x + 1;
            }  // Clip right
            if (y2 > _max_y)
            {
              h = _max_y - y + 1;
            }  // Clip bottom
            writeFillRectPreclipped(x, y, w, h, color);
          }
        }
      }
    }
  }
}

/**************************************************************************/
/*!
  @brief  Write a rectangle completely with one color, overwrite in subclasses if startWrite is defined!
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  w       Width in pixels
  @param  h       Height in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled)
  {
    ppaFill(x, y, w, h, color);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    // Overwrite in subclasses if desired!
    for (int16_t i = y; i < y + h; ++i)
      writeFastHLine(x, i, w, color);
  }
}

/**************************************************************************/
/*!
  @brief  End a display-writing routine, overwrite in subclasses if startWrite is defined!
*/
/**************************************************************************/
GFX_INLINE void Arduino_GFX::endWrite()
{
}

/**************************************************************************/
/*!
  @brief  Draw a perfectly vertical line (this is often optimized in a subclass!)
  @param  x       Top-most x coordinate
  @param  y       Top-most y coordinate
  @param  h       Height in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  startWrite();
  writeFastVLine(x, y, h, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a perfectly horizontal line (this is often optimized in a subclass!)
  @param  x       Left-most x coordinate
  @param  y       Left-most y coordinate
  @param  w       Width in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  startWrite();
  writeFastHLine(x, y, w, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Fill a rectangle completely with one color. Update in subclasses if desired!
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  w       Width in pixels
  @param  h       Height in pixels
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  startWrite();
  writeFillRect(x, y, w, h, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Fill the screen completely with one color. Update in subclasses if desired!
  @param  color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::fillScreen(uint16_t color)
{
  fillRect(0, 0, _width, _height, color);
}

/**************************************************************************/
/*!
  @brief  Draw a line
  @param  x0      Start point x coordinate
  @param  y0      Start point y coordinate
  @param  x1      End point x coordinate
  @param  y1      End point y coordinate
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  // Update in subclasses if desired!
  startWrite();
  writeLine(x0, y0, x1, y1, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a circle outline
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  r       Radius of circle
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
  startWrite();
  writeEllipseHelper(x, y, r, r, 0xf, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Quarter-ellipse drawer, used to do circles and roundrects
  @param  x           Center-point x coordinate
  @param  y           Center-point y coordinate
  @param  rx          radius of x coordinate
  @param  ry          radius of y coordinate
  @param  cornername  Mask bit #1 or bit #2 to indicate which quarters of the circle we're doing
  @param  color       16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::writeEllipseHelper(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t cornername, uint16_t color)
{
  if (rx < 0 || ry < 0 || ((rx == 0) && (ry == 0)))
  {
    return;
  }
  if (ry == 0)
  {
    writeFastHLine(x - rx, y, (ry << 2) + 1, color);
    return;
  }
  if (rx == 0)
  {
    writeFastVLine(x, y - ry, (rx << 2) + 1, color);
    return;
  }

  int32_t xt, yt, s, i;
  int32_t rx2 = rx * rx;
  int32_t ry2 = ry * ry;

  i = -1;
  xt = 0;
  yt = ry;
  s = (ry2 << 1) + rx2 * (1 - (ry << 1));
  do
  {
    while (s < 0)
      s += ry2 * ((++xt << 2) + 2);
    if (cornername & 0x1)
    {
      writeFastHLine(x - xt, y - yt, xt - i, color);
    }
    if (cornername & 0x2)
    {
      writeFastHLine(x + i + 1, y - yt, xt - i, color);
    }
    if (cornername & 0x4)
    {
      writeFastHLine(x + i + 1, y + yt, xt - i, color);
    }
    if (cornername & 0x8)
    {
      writeFastHLine(x - xt, y + yt, xt - i, color);
    }
    i = xt;
    s -= (--yt) * rx2 << 2;
  } while (ry2 * xt <= rx2 * yt);

  i = -1;
  yt = 0;
  xt = rx;
  s = (rx2 << 1) + ry2 * (1 - (rx << 1));
  do
  {
    while (s < 0)
      s += rx2 * ((++yt << 2) + 2);
    if (cornername & 0x1)
    {
      writeFastVLine(x - xt, y - yt, yt - i, color);
    }
    if (cornername & 0x2)
    {
      writeFastVLine(x + xt, y - yt, yt - i, color);
    }
    if (cornername & 0x4)
    {
      writeFastVLine(x + xt, y + i + 1, yt - i, color);
    }
    if (cornername & 0x8)
    {
      writeFastVLine(x - xt, y + i + 1, yt - i, color);
    }
    i = yt;
    s -= (--xt) * ry2 << 2;
  } while (rx2 * yt <= ry2 * xt);
}

/**************************************************************************/
/*!
  @brief  Draw a circle with filled color
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  r       Radius of circle
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
  startWrite();
  writeFillEllipseHelper(x, y, r, r, 3, 0, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Quarter-circle drawer with fill, used for circles and roundrects
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  rx      Radius of x coordinate
  @param  ry      Radius of y coordinate
  @param  corners Mask bits indicating which quarters we're doing
  @param  delta   Offset from center-point, used for round-rects
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::writeFillEllipseHelper(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t corners, int16_t delta, uint16_t color)
{
  if (rx < 0 || ry < 0 || ((rx == 0) && (ry == 0)))
  {
    return;
  }
  if (ry == 0)
  {
    writeFastHLine(x - rx, y, (ry << 2) + 1, color);
    return;
  }
  if (rx == 0)
  {
    writeFastVLine(x, y - ry, (rx << 2) + 1, color);
    return;
  }

  int32_t xt, yt, i;
  int32_t rx2 = (int32_t)rx * rx;
  int32_t ry2 = (int32_t)ry * ry;
  int32_t s;

  writeFastHLine(x - rx, y, (rx << 1) + 1, color);
  i = 0;
  yt = 0;
  xt = rx;
  s = (rx2 << 1) + ry2 * (1 - (rx << 1));
  do
  {
    while (s < 0)
    {
      s += rx2 * ((++yt << 2) + 2);
    }
    if (corners & 1)
    {
      writeFillRect(x - xt, y - yt, (xt << 1) + 1 + delta, yt - i, color);
    }
    if (corners & 2)
    {
      writeFillRect(x - xt, y + i + 1, (xt << 1) + 1 + delta, yt - i, color);
    }
    i = yt;
    s -= (--xt) * ry2 << 2;
  } while (rx2 * yt <= ry2 * xt);

  xt = 0;
  yt = ry;
  s = (ry2 << 1) + rx2 * (1 - (ry << 1));
  do
  {
    while (s < 0)
    {
      s += ry2 * ((++xt << 2) + 2);
    }
    if (corners & 1)
    {
      writeFastHLine(x - xt, y - yt, (xt << 1) + 1 + delta, color);
    }
    if (corners & 2)
    {
      writeFastHLine(x - xt, y + yt, (xt << 1) + 1 + delta, color);
    }
    s -= (--yt) * rx2 << 2;
  } while (ry2 * xt <= rx2 * yt);
}

/**************************************************************************/
/*!
  @brief  Draw an ellipse outline
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  rx      radius of x coordinate
  @param  ry      radius of y coordinate
  @param  start   degree of ellipse start
  @param  end     degree of ellipse end
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawEllipse(int16_t x, int16_t y, int16_t rx, int16_t ry, uint16_t color)
{
  startWrite();
  writeEllipseHelper(x, y, rx, ry, 0xf, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw an ellipse with filled color
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  rx      radius of x coordinate
  @param  ry      radius of y coordinate
  @param  start   degree of ellipse start
  @param  end     degree of ellipse end
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::fillEllipse(int16_t x, int16_t y, int16_t rx, int16_t ry, uint16_t color)
{
  startWrite();
  writeFillEllipseHelper(x, y, rx, ry, 3, 0, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw an arc outline
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  r1      Outer radius of arc
  @param  r2      Inner radius of arc
  @param  start   degree of arc start
  @param  end     degree of arc end
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color)
{
  if (r1 < r2)
  {
    _swap_int16_t(r1, r2);
  }
  if (r1 < 1)
  {
    r1 = 1;
  }
  if (r2 < 1)
  {
    r2 = 1;
  }
  bool equal = fabsf(start - end) < FLT_EPSILON;
  start = fmodf(start, 360);
  end = fmodf(end, 360);
  if (start < 0)
    start += 360.0;
  if (end < 0)
    end += 360.0;

  startWrite();
  writeFillArcHelper(x, y, r1, r2, start, start, color);
  writeFillArcHelper(x, y, r1, r2, end, end, color);
  if (!equal && (fabsf(start - end) <= 0.0001))
  {
    start = .0;
    end = 360.0;
  }
  writeFillArcHelper(x, y, r1, r1, start, end, color);
  writeFillArcHelper(x, y, r2, r2, start, end, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw an arc with filled color
  @param  x       Center-point x coordinate
  @param  y       Center-point y coordinate
  @param  r1      Outer radius of arc
  @param  r2      Inner radius of arc
  @param  start   degree of arc start
  @param  end     degree of arc end
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::fillArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color)
{
  if (r1 < r2)
  {
    _swap_int16_t(r1, r2);
  }
  if (r1 < 1)
  {
    r1 = 1;
  }
  if (r2 < 1)
  {
    r2 = 1;
  }
  bool equal = fabsf(start - end) < FLT_EPSILON;
  start = fmodf(start, 360);
  end = fmodf(end, 360);
  if (start < 0)
    start += 360.0;
  if (end < 0)
    end += 360.0;
  if (!equal && (fabsf(start - end) <= 0.0001))
  {
    start = .0;
    end = 360.0;
  }

  startWrite();
  writeFillArcHelper(x, y, r1, r2, start, end, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Arc drawer with fill
  @param  cx      Center-point x coordinate
  @param  cy      Center-point y coordinate
  @param  oradius Outer radius of arc
  @param  iradius Inner radius of arc
  @param  start   degree of arc start
  @param  end     degree of arc end
  @param  color   16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_GFX::writeFillArcHelper(int16_t cx, int16_t cy, int16_t oradius, int16_t iradius, float start, float end, uint16_t color)
{
  if ((start == 90.0) || (start == 180.0) || (start == 270.0) || (start == 360.0))
  {
    start -= 0.1;
  }

  if ((end == 90.0) || (end == 180.0) || (end == 270.0) || (end == 360.0))
  {
    end -= 0.1;
  }

  float s_cos = (cos(start * DEGTORAD));
  float e_cos = (cos(end * DEGTORAD));
  float sslope = s_cos / (sin(start * DEGTORAD));
  float eslope = e_cos / (sin(end * DEGTORAD));
  float swidth = 0.5 / s_cos;
  float ewidth = -0.5 / e_cos;
  --iradius;
  int32_t ir2 = iradius * iradius + iradius;
  int32_t or2 = oradius * oradius + oradius;

  bool start180 = !(start < 180.0);
  bool end180 = end < 180.0;
  bool reversed = start + 180.0 < end || (end < start && start < end + 180.0);

  int32_t xs = -oradius;
  int32_t y = -oradius;
  int32_t ye = oradius;
  int32_t xe = oradius + 1;
  if (!reversed)
  {
    if ((end >= 270 || end < 90) && (start >= 270 || start < 90))
    {
      xs = 0;
    }
    else if (end < 270 && end >= 90 && start < 270 && start >= 90)
    {
      xe = 1;
    }
    if (end >= 180 && start >= 180)
    {
      ye = 0;
    }
    else if (end < 180 && start < 180)
    {
      y = 0;
    }
  }
  do
  {
    int32_t y2 = y * y;
    int32_t x = xs;
    if (x < 0)
    {
      while (x * x + y2 >= or2)
      {
        ++x;
      }
      if (xe != 1)
      {
        xe = 1 - x;
      }
    }
    float ysslope = (y + swidth) * sslope;
    float yeslope = (y + ewidth) * eslope;
    int32_t len = 0;
    do
    {
      bool flg1 = start180 != (x <= ysslope);
      bool flg2 = end180 != (x <= yeslope);
      int32_t distance = x * x + y2;
      if (distance >= ir2 && ((flg1 && flg2) || (reversed && (flg1 || flg2))) && x != xe && distance < or2)
      {
        ++len;
      }
      else
      {
        if (len)
        {
          writeFastHLine(cx + x - len, cy + y, len, color);
          len = 0;
        }
        if (distance >= or2)
          break;
        if (x < 0 && distance < ir2)
        {
          x = -x;
        }
      }
    } while (++x <= xe);
  } while (++y <= ye);
}

/**************************************************************************/
/*!
  @brief  Draw a rectangle with no fill color
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  w       Width in pixels
  @param  h       Height in pixels
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  startWrite();
  writeFastHLine(x, y, w, color);
  writeFastHLine(x, y + h - 1, w, color);
  writeFastVLine(x, y, h, color);
  writeFastVLine(x + w - 1, y, h, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a rounded rectangle with no fill color
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  w       Width in pixels
  @param  h       Height in pixels
  @param  r       Radius of corner rounding
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
  int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  startWrite();
  writeFastHLine(x + r, y, w - 2 * r, color);          // Top
  writeFastHLine(x + r, y + h - 1, w - 2 * r, color);  // Bottom
  writeFastVLine(x, y + r, h - 2 * r, color);          // Left
  writeFastVLine(x + w - 1, y + r, h - 2 * r, color);  // Right
  // draw four corners
  writeEllipseHelper(x + r, y + r, r, r, 1, color);
  writeEllipseHelper(x + w - r - 1, y + r, r, r, 2, color);
  writeEllipseHelper(x + w - r - 1, y + h - r - 1, r, r, 4, color);
  writeEllipseHelper(x + r, y + h - r - 1, r, r, 8, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a rounded rectangle with fill color
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  w       Width in pixels
  @param  h       Height in pixels
  @param  r       Radius of corner rounding
  @param  color   16-bit 5-6-5 Color to draw/fill with
*/
/**************************************************************************/
void Arduino_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
  int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  startWrite();
  writeFillRect(x, y + r, w, h - (r << 1), color);
  // draw four corners
  writeFillEllipseHelper(x + r, y + r, r, r, 1, w - 2 * r - 1, color);
  writeFillEllipseHelper(x + r, y + h - r - 1, r, r, 2, w - 2 * r - 1, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a triangle with no fill color
  @param  x0      Vertex #0 x coordinate
  @param  y0      Vertex #0 y coordinate
  @param  x1      Vertex #1 x coordinate
  @param  y1      Vertex #1 y coordinate
  @param  x2      Vertex #2 x coordinate
  @param  y2      Vertex #2 y coordinate
  @param  color   16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_GFX::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  startWrite();
  writeLine(x0, y0, x1, y1, color);
  writeLine(x1, y1, x2, y2, color);
  writeLine(x2, y2, x0, y0, color);
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a triangle with color-fill
  @param  x0      Vertex #0 x coordinate
  @param  y0      Vertex #0 y coordinate
  @param  x1      Vertex #1 x coordinate
  @param  y1      Vertex #1 y coordinate
  @param  x2      Vertex #2 x coordinate
  @param  y2      Vertex #2 y coordinate
  @param  color   16-bit 5-6-5 Color to fill/draw with
*/
/**************************************************************************/
void Arduino_GFX::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1)
  {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2)
  {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1)
  {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }

  startWrite();
  if (y0 == y2)
  {  // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    writeFastHLine(a, y0, b - a + 1, color);
    endWrite();
    return;
  }

  int16_t
      dx01 = x1 - x0,
      dy01 = y1 - y0,
      dx02 = x2 - x0,
      dy02 = y2 - y0,
      dx12 = x2 - x1,
      dy12 = y2 - y1;
  int32_t
      sa = 0,
      sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
  {
    last = y1;  // Include y1 scanline
  }
  else
  {
    last = y1 - 1;  // Skip it
  }

  for (y = y0; y <= last; y++)
  {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
    {
      _swap_int16_t(a, b);
    }
    writeFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++)
  {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
    {
      _swap_int16_t(a, b);
    }
    writeFastHLine(a, y, b - a + 1, color);
  }
  endWrite();
}

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/**************************************************************************/
/*!
  @brief  Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  bitmap  byte array with 16-bit color bitmap
  @param  w       Width of bitmap in pixels
  @param  h       Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_GFX::draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h)
{
  int32_t offset = 0;
  startWrite();
  for (int16_t j = 0; j < h; j++, y++)
  {
    for (int16_t i = 0; i < w; i++)
    {
      writePixel(x + i, y, bitmap[offset++]);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
  @brief  Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
  @param  x       Top left corner x coordinate
  @param  y       Top left corner y coordinate
  @param  bitmap  byte array with 16-bit color bitmap
  @param  transparent_color
  @param  w       Width of bitmap in pixels
  @param  h       Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_GFX::draw16bitRGBBitmapWithTranColor(
    int16_t x,
    int16_t y,
    const uint16_t* bitmap,
    uint16_t transparent_color,
    int16_t w,
    int16_t h)
{
  int32_t offset = 0;
  uint16_t color;
  startWrite();
  for (int16_t j = 0; j < h; j++, y++)
  {
    for (int16_t i = 0; i < w; i++)
    {
      color = bitmap[offset++];
      if (color != transparent_color)
      {
        writePixel(x + i, y, color);
      }
    }
  }
  endWrite();
}

uint16_t Arduino_GFX::u8g2_font_get_word(const uint8_t* font, uint8_t offset)
{
  uint16_t pos;
  font += offset;
  pos = pgm_read_byte(font);
  font++;
  pos <<= 8;
  pos += pgm_read_byte(font);

  return pos;
}

uint8_t Arduino_GFX::u8g2_font_decode_get_unsigned_bits(uint8_t cnt)
{
  uint8_t val;
  uint8_t bit_pos = _u8g2_decode_bit_pos;
  uint8_t bit_pos_plus_cnt;

  val = pgm_read_byte(_u8g2_decode_ptr);

  val >>= bit_pos;
  bit_pos_plus_cnt = bit_pos;
  bit_pos_plus_cnt += cnt;
  if (bit_pos_plus_cnt >= 8)
  {
    uint8_t s = 8;
    s -= bit_pos;
    _u8g2_decode_ptr++;
    val |= pgm_read_byte(_u8g2_decode_ptr) << (s);
    bit_pos_plus_cnt -= 8;
  }
  val &= (1U << cnt) - 1;

  _u8g2_decode_bit_pos = bit_pos_plus_cnt;

  return val;
}

int8_t Arduino_GFX::u8g2_font_decode_get_signed_bits(uint8_t cnt)
{
  int8_t v, d;
  v = (int8_t)u8g2_font_decode_get_unsigned_bits(cnt);
  d = 1;
  cnt--;
  d <<= cnt;
  v -= d;

  return v;
}

void Arduino_GFX::u8g2_font_decode_len(uint8_t len, uint8_t is_foreground, uint16_t color, uint16_t bg)
{
  uint8_t cnt;     /* total number of remaining pixels, which have to be drawn */
  uint8_t rem;     /* remaining pixel to the right edge of the glyph */
  uint8_t current; /* number of pixels, which need to be drawn for the draw procedure */
  /* current is either equal to cnt or equal to rem */

  /* target position on the screen */
  uint16_t x, y;

  cnt = len;

  /* get the local position */
  uint8_t lx = _u8g2_dx;
  uint8_t ly = _u8g2_dy;

  int16_t curW;
  for (;;)
  {
    /* calculate the number of pixel to the right edge of the glyph */
    rem = _u8g2_char_width;
    rem -= lx;

    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if (cnt < rem)
      current = cnt;

    /* now draw the line, but apply the rotation around the glyph target position */
    // u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);

    if (textsize_x == 1 && textsize_y == 1)
    {
      /* get target position */
      x = _u8g2_target_x + lx;
      y = _u8g2_target_y + ly;

      /* draw foreground and background (if required) */
      if ((x <= _max_text_x) && (y <= _max_text_y) && (y >= _min_text_y))
      {
        curW = current;
        if ((x + curW - 1) > _max_text_x)
        {
          curW = _max_text_x - x + 1;
        }
        if (x < _min_text_x)
        {
          // Clip left margin, i.e. characters partially to the left of
          // edge of the text bound
          curW = max(0, (curW - (_min_text_x - x)));
          x = _min_text_x;
        }
        if (is_foreground)
        {
          writeFillRect(x, y, curW, 1, color);
        }
        else if (bg != color)
        {
          writeFillRect(x, y, curW, 1, bg);
        }
      }
    }
    else
    {
      /* get target position */
      x = _u8g2_target_x + (lx * textsize_x);
      y = _u8g2_target_y + (ly * textsize_y);

      /* draw foreground and background (if required) */
      if (((x + textsize_x - 1) <= _max_text_x) && ((y + textsize_y - 1) <= _max_text_y) && (y + textsize_y - 1) >= _min_text_y)
      {
        curW = current * textsize_x;
        while ((x + curW - 1) > _max_text_x)
        {
          curW -= textsize_x;
        }
        if (x < _min_text_x)
        {
          // Clip left margin, i.e. characters partially to the left of
          // edge of the text bound
          curW = max(0, (curW - (_min_text_x - x)));
          x = _min_text_x;
        }
        if (is_foreground)
        {
          writeFillRect(x, y, curW - text_pixel_margin,
                        textsize_y - text_pixel_margin, color);
        }
        else if (bg != color)
        {
          writeFillRect(x, y, curW - text_pixel_margin,
                        textsize_y - text_pixel_margin, bg);
        }
      }
    }

    /* check, whether the end of the run length code has been reached */
    if (cnt < rem)
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;

  _u8g2_dx = lx;
  _u8g2_dy = ly;
}

void Arduino_GFX::flushMainBuff()
{
  log_e("Відсутня реалізація методу");
}

void Arduino_GFX::duplicateMainBuff()
{
  log_e("Відсутня реалізація методу");
}

void Arduino_GFX::flushSecondBuff()
{
  log_e("Відсутня реалізація методу");
}

void Arduino_GFX::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  log_e("Відсутня реалізація методу");
}

// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

// Draw a character
/**************************************************************************/
/*!
  @brief  Draw a single character
  @param  x       Bottom left corner x coordinate
  @param  y       Bottom left corner y coordinate
  @param  c       The 8-bit font-indexed character (likely ascii)
  @param  color   16-bit 5-6-5 Color to draw chraracter with
  @param  bg      16-bit 5-6-5 Color to fill background with (if same as color, no background)
*/
/**************************************************************************/
void Arduino_GFX::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg)
{
  int16_t block_w, block_h, curX, curY, curW, curH;

  if (u8g2Font)
  {
    _u8g2_target_y = y - ((_u8g2_char_height + _u8g2_char_y) * textsize_y);
    if (_u8g2_target_y > _max_text_y)
    {
      return;
    }

    if ((_u8g2_decode_ptr) && (_u8g2_char_width > 0))
    {
      uint8_t a, b;

      _u8g2_target_x = x + (_u8g2_char_x * textsize_x);
      // log_d("_u8g2_target_x: %d, _u8g2_target_y: %d", _u8g2_target_x, _u8g2_target_y);

      /* reset local x/y position */
      _u8g2_dx = 0;
      _u8g2_dy = 0;
      /* decode glyph */
      startWrite();
      for (;;)
      {
        a = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_0);
        b = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_1);
        // log_d("a: %d, b: %d", a, b);
        do
        {
          u8g2_font_decode_len(a, 0, color, bg);
          u8g2_font_decode_len(b, 1, color, bg);
        } while (u8g2_font_decode_get_unsigned_bits(1) != 0);

        if (_u8g2_dy >= _u8g2_char_height)
          break;
      }
      endWrite();
    }
  }
  else  // glcdfont
  {
    block_w = 6 * textsize_x;
    block_h = 8 * textsize_y;
    if (
        (x > _max_text_x) ||                  // Clip right
        (y > _max_text_y) ||                  // Clip bottom
        ((x + block_w - 1) < _min_text_x) ||  // Clip left
        ((y + block_h - 1) < _min_text_y)     // Clip top
    )
    {
      return;
    }

    startWrite();
    if (textsize_x == 1 && textsize_y == 1)
    {
      curX = x;
      for (int8_t i = 0; i < 5; ++i, ++curX)  // Char bitmap = 5 columns
      {
        uint8_t line = pgm_read_byte(&font[c * 5 + i]);
        if (curX <= _max_text_x)
        {
          curY = y;
          for (int8_t j = 0; j < 8; ++j, ++curY, line >>= 1)
          {
            if (curY <= _max_text_y)
            {
              if (line & 1)
              {
                writePixelPreclipped(curX, curY, color);
              }
              else if (bg != color)
              {
                writePixelPreclipped(curX, curY, bg);
              }
            }
          }
        }
      }
      if (bg != color)  // If opaque, draw vertical line for last column
      {
        curX = x + 5;
        if (curX <= _max_text_x)
        {
          curH = ((y + 8 - 1) <= _max_text_y) ? 8 : (_max_text_y - y + 1);
          writeFastVLine(curX, y, curH, bg);
        }
      }
    }
    else  // (textsize_x > 1 || textsize_y > 1)
    {
      curX = x;
      for (int8_t i = 0; i < 5; ++i, curX += textsize_x)  // Char bitmap = 5 columns
      {
        if ((curX + textsize_x - 1) <= _max_text_x)
        {
          uint8_t line = pgm_read_byte(&font[c * 5 + i]);
          curY = y;
          for (int8_t j = 0; j < 8; j++, line >>= 1, curY += textsize_y)
          {
            if ((curY + textsize_y - 1) <= _max_text_y)
            {
              if (line & 1)
              {
                if (text_pixel_margin > 0)
                {
                  writeFillRect(curX, y + j * textsize_y, textsize_x - text_pixel_margin, textsize_y - text_pixel_margin, color);
                  writeFillRect(curX + textsize_x - text_pixel_margin, y + j * textsize_y, text_pixel_margin, textsize_y, bg);
                  writeFillRect(curX, y + ((j + 1) * textsize_y) - text_pixel_margin, textsize_x - text_pixel_margin, text_pixel_margin, bg);
                }
                else
                {
                  writeFillRect(curX, curY, textsize_x, textsize_y, color);
                }
              }
              else if (bg != color)
              {
                writeFillRect(curX, curY, textsize_x, textsize_y, bg);
              }
            }
          }
        }
      }
      if (bg != color)  // If opaque, draw vertical line for last column
      {
        curX = x + 5 * textsize_x;
        if ((curX + textsize_x - 1) <= _max_text_x)
        {
          curH = 8 * textsize_y;
          while ((y + curH - 1) > _max_text_y)
          {
            curH -= textsize_y;
          }
          writeFillRect(curX, y, textsize_x, curH, bg);
        }
      }
    }
    endWrite();
  }
}

void Arduino_GFX::setPPAState(bool state)
{
#if CONFIG_IDF_TARGET_ESP32P4
  _ppa_enabled = state;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
}

bool Arduino_GFX::isPPAEnabled() const
{
#if CONFIG_IDF_TARGET_ESP32P4
  return _ppa_enabled;
#else
  return false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
}

void Arduino_GFX::setTextBound(int16_t x, int16_t y, int16_t w, int16_t h)
{
  _min_text_x = x;
  _min_text_y = y;
  _max_text_x = x + w - 1;
  _max_text_y = y + h - 1;
}

void Arduino_GFX::resetTextBound()
{
  _min_text_x = 0;
  _min_text_y = 0;
  _max_text_x = _max_x;
  _max_text_y = _max_y;
}

/**************************************************************************/
/*!
  @brief  Print one byte/character of data, used to support print()
  @param  c   The 8-bit ascii character to write
*/
/**************************************************************************/
size_t Arduino_GFX::write(uint8_t c)
{
  if (u8g2Font)
  {
    _u8g2_decode_ptr = 0;

    if (_enableUTF8Print)
    {
      if (_utf8_state == 0)
      {
        if (c >= 0xfc) /* 6 byte sequence */
        {
          _utf8_state = 5;
          c &= 1;
        }
        else if (c >= 0xf8)
        {
          _utf8_state = 4;
          c &= 3;
        }
        else if (c >= 0xf0)
        {
          _utf8_state = 3;
          c &= 7;
        }
        else if (c >= 0xe0)
        {
          _utf8_state = 2;
          c &= 15;
        }
        else if (c >= 0xc0)
        {
          _utf8_state = 1;
          c &= 0x01f;
        }
        _encoding = c;
      }
      else
      {
        _utf8_state--;
        /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
        _encoding <<= 6;
        c &= 0x03f;
        _encoding |= c;
      }
    }  // _enableUTF8Print
    else
    {
      _encoding = c;
    }

    if (_utf8_state == 0)
    {
      if (_encoding == '\n')
      {
        cursor_x = _min_text_x;
        cursor_y += (int16_t)textsize_y * _u8g2_max_char_height;
      }
      else if (_encoding != '\r')
      {  // Ignore carriage returns
        const uint8_t* font = u8g2Font;
        const uint8_t* glyph_data = 0;

        // extract from u8g2_font_get_glyph_data()
        font += 23;  // U8G2_FONT_DATA_STRUCT_SIZE
        if (_encoding <= 255)
        {
          if (_encoding >= 'a')
          {
            font += _u8g2_start_pos_lower_a;
          }
          else if (_encoding >= 'A')
          {
            font += _u8g2_start_pos_upper_A;
          }

          for (;;)
          {
            if (pgm_read_byte(font + 1) == 0)
              break;
            if (pgm_read_byte(font) == _encoding)
            {
              glyph_data = font + 2; /* skip encoding and glyph size */
            }
            font += pgm_read_byte(font + 1);
          }
        }
        else
        {
          uint16_t e;
          font += _u8g2_start_pos_unicode;
          const uint8_t* unicode_lookup_table = font;

          /* issue 596: search for the glyph start in the unicode lookup table */
          do
          {
            font += u8g2_font_get_word(unicode_lookup_table, 0);
            e = u8g2_font_get_word(unicode_lookup_table, 2);
            unicode_lookup_table += 4;
          } while (e < _encoding);

          for (;;)
          {
            e = u8g2_font_get_word(font, 0);

            if (e == 0)
              break;

            if (e == _encoding)
            {
              glyph_data = font + 3; /* skip encoding and glyph size */
              break;
            }
            font += pgm_read_byte(font + 2);
          }
        }

        if (glyph_data)
        {
          // u8g2_font_decode_glyph
          _u8g2_decode_ptr = glyph_data;
          _u8g2_decode_bit_pos = 0;

          _u8g2_char_width = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_char_width);
          _u8g2_char_height = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_char_height);
          _u8g2_char_x = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_char_x);
          _u8g2_char_y = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_char_y);
          _u8g2_delta_x = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_delta_x);
          // log_d("c: %c, _encoding: %d, _u8g2_char_width: %d, _u8g2_char_height: %d, _u8g2_char_x: %d, _u8g2_char_y: %d, _u8g2_delta_x: %d",
          //       c, _encoding, _u8g2_char_width, _u8g2_char_height, _u8g2_char_x, _u8g2_char_y, _u8g2_delta_x);

          if (_u8g2_char_width > 0)
          {
            if (wrap && ((cursor_x + (textsize_x * _u8g2_char_width) - 1) > _max_text_x))
            {
              cursor_x = _min_text_x;
              cursor_y += (int16_t)textsize_y * _u8g2_max_char_height;
            }
          }

          drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor);
          cursor_x += (int16_t)textsize_x * _u8g2_delta_x;
        }
      }
    }
  }
  else  // glcdfont
  {
    if (c == '\n')
    {                              // Newline?
      cursor_x = _min_text_x;      // Reset x to zero,
      cursor_y += textsize_y * 8;  // advance y one line
    }
    else if (c != '\r')  // Normal char; ignore carriage returns
    {
      if (wrap && ((cursor_x + (textsize_x * 6) - 1) > _max_text_x))  // Off right?
      {
        cursor_x = _min_text_x;      // Reset x to zero,
        cursor_y += textsize_y * 8;  // advance y one line
      }
      drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor);
      cursor_x += textsize_x * 6;  // Advance x one char
    }
  }
  return 1;
}

/**************************************************************************/
/*!
  @brief  Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
  @param  s   Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
/**************************************************************************/
void Arduino_GFX::setTextSize(uint8_t s)
{
  setTextSize(s, s, 0);
}

/**************************************************************************/
/*!
  @brief  Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
  @param  s_x Desired text width magnification level in X-axis. 1 is default
  @param  s_y Desired text width magnification level in Y-axis. 1 is default
*/
/**************************************************************************/
void Arduino_GFX::setTextSize(uint8_t s_x, uint8_t s_y)
{
  setTextSize(s_x, s_y, 0);
}

/**************************************************************************/
/*!
  @brief  Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
  @param  s_x             Desired text width magnification level in X-axis. 1 is default
  @param  s_y             Desired text width magnification level in Y-axis. 1 is default
  @param  pixel_margin    Margin for each text pixel. 0 is default
*/
/**************************************************************************/
void Arduino_GFX::setTextSize(uint8_t s_x, uint8_t s_y, uint8_t pixel_margin)
{
  text_pixel_margin = ((pixel_margin < s_x) && (pixel_margin < s_y)) ? pixel_margin : 0;
  textsize_x = (s_x > 0) ? s_x : 1;
  textsize_y = (s_y > 0) ? s_y : 1;
}

/**************************************************************************/
/*!
  @brief  Set rotation setting for display
  @param  r   0 thru 3 corresponding to 4 cardinal rotations
*/
/**************************************************************************/
void Arduino_GFX::setRotation(uint8_t r)
{
  _rotation = (r & 7);
  switch (_rotation)
  {
    case 7:
    case 5:
    case 3:
    case 1:
      _width = HEIGHT;
      _height = WIDTH;
      break;
    case 6:
    case 4:
    case 2:
    default:  // case 0:
      _width = WIDTH;
      _height = HEIGHT;
      break;
  }

  _max_x = _width - 1;   ///< x zero base bound
  _max_y = _height - 1;  ///< y zero base bound

  // reset textBound after setRotation()
  setTextBound(0, 0, _width, _height);
}

void Arduino_GFX::setFont(const uint8_t* font)
{
  u8g2Font = font;

  // extract from u8g2_read_font_info()
  /* offset 0 */
  _u8g2_glyph_cnt = pgm_read_byte(font);
  // uint8_t bbx_mode = pgm_read_byte(font + 1);
  _u8g2_bits_per_0 = pgm_read_byte(font + 2);
  _u8g2_bits_per_1 = pgm_read_byte(font + 3);
  // log_d("_u8g2_glyph_cnt: %d, bbx_mode: %d, _u8g2_bits_per_0: %d, _u8g2_bits_per_1: %d",
  //       _u8g2_glyph_cnt,
  //       bbx_mode,
  //       _u8g2_bits_per_0,
  //       _u8g2_bits_per_1);

  /* offset 4 */
  _u8g2_bits_per_char_width = pgm_read_byte(font + 4);
  _u8g2_bits_per_char_height = pgm_read_byte(font + 5);
  _u8g2_bits_per_char_x = pgm_read_byte(font + 6);
  _u8g2_bits_per_char_y = pgm_read_byte(font + 7);
  _u8g2_bits_per_delta_x = pgm_read_byte(font + 8);
  // log_d("_u8g2_bits_per_char_width: %d, _u8g2_bits_per_char_height: %d, _u8g2_bits_per_char_x: %d, _u8g2_bits_per_char_y: %d, _u8g2_bits_per_delta_x: %d",
  //       _u8g2_bits_per_char_width,
  //       _u8g2_bits_per_char_height,
  //       _u8g2_bits_per_char_x,
  //       _u8g2_bits_per_char_y,
  //       _u8g2_bits_per_delta_x);

  /* offset 9 */
  _u8g2_max_char_width = pgm_read_byte(font + 9);
  _u8g2_max_char_height = pgm_read_byte(font + 10);
  // int8_t x_offset = pgm_read_byte(font + 11);
  // int8_t y_offset = pgm_read_byte(font + 12);
  // log_d("_u8g2_max_char_width: %d, _u8g2_max_char_height: %d, x_offset: %d, y_offset: %d",
  //       _u8g2_max_char_width, _u8g2_max_char_height, x_offset, y_offset);

  /* offset 13 */
  // int8_t ascent_A = pgm_read_byte(font + 13);
  // int8_t descent_g = pgm_read_byte(font + 14);
  // int8_t ascent_para = pgm_read_byte(font + 15);
  // int8_t descent_para = pgm_read_byte(font + 16);
  // log_d("ascent_A: %d, descent_g: %d, ascent_para: %d, descent_para: %d",
  //       ascent_A, descent_g, ascent_para, descent_para);

  /* offset 17 */
  _u8g2_start_pos_upper_A = u8g2_font_get_word(font, 17);
  _u8g2_start_pos_lower_a = u8g2_font_get_word(font, 19);
  _u8g2_start_pos_unicode = u8g2_font_get_word(font, 21);
  _u8g2_first_char = pgm_read_byte(font + 23);
  // log_d("_u8g2_start_pos_upper_A: %d, _u8g2_start_pos_lower_a: %d, _u8g2_start_pos_unicode: %d, _u8g2_first_char: %d",
  //       _u8g2_start_pos_upper_A, _u8g2_start_pos_lower_a, _u8g2_start_pos_unicode, _u8g2_first_char);
}

void Arduino_GFX::setUTF8Print(bool isEnable)
{
  _enableUTF8Print = isEnable;
}

/**************************************************************************/
/*!
  @brief  Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param  str The ascii string to measure
  @param  x   The current cursor X
  @param  y   The current cursor Y
  @param  x1  The boundary X coordinate, set by function
  @param  y1  The boundary Y coordinate, set by function
  @param  w   The boundary width, set by function
  @param  h   The boundary height, set by function
*/
/**************************************************************************/
void Arduino_GFX::getTextBounds(const char* str, int16_t x, int16_t y, int16_t& x1, int16_t& y1, uint16_t& w, uint16_t& h)
{
  uint8_t c;  // Current character

  x1 = x;
  y1 = y;
  w = h = 0;

  int16_t minx = _max_text_x, miny = _max_text_y, maxx = _min_text_x, maxy = _min_text_y;
  int16_t curr_x = x, curr_y = y;

  while ((c = *str++))
  {
    if (u8g2Font)
    {
      _u8g2_decode_ptr = 0;

      if (_enableUTF8Print)
      {
        if (_utf8_state == 0)
        {
          if (c >= 0xfc) /* 6 byte sequence */
          {
            _utf8_state = 5;
            c &= 1;
          }
          else if (c >= 0xf8)
          {
            _utf8_state = 4;
            c &= 3;
          }
          else if (c >= 0xf0)
          {
            _utf8_state = 3;
            c &= 7;
          }
          else if (c >= 0xe0)
          {
            _utf8_state = 2;
            c &= 15;
          }
          else if (c >= 0xc0)
          {
            _utf8_state = 1;
            c &= 0x01f;
          }
          _encoding = c;
        }
        else
        {
          _utf8_state--;
          /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
          _encoding <<= 6;
          c &= 0x03f;
          _encoding |= c;
        }
      }  // _enableUTF8Print
      else
      {
        _encoding = c;
      }

      if (_utf8_state == 0)
      {
        if (_encoding == '\n')
        {
          curr_x = _min_text_x;
          curr_y += (int16_t)textsize_y * _u8g2_max_char_height;
        }
        else if (_encoding != '\r')
        {  // Ignore carriage returns
          const uint8_t* font = u8g2Font;
          const uint8_t* glyph_data = 0;

          // extract from u8g2_font_get_glyph_data()
          font += 23;  // U8G2_FONT_DATA_STRUCT_SIZE
          if (_encoding <= 255)
          {
            if (_encoding >= 'a')
            {
              font += _u8g2_start_pos_lower_a;
            }
            else if (_encoding >= 'A')
            {
              font += _u8g2_start_pos_upper_A;
            }

            for (;;)
            {
              if (pgm_read_byte(font + 1) == 0)
                break;
              if (pgm_read_byte(font) == _encoding)
              {
                glyph_data = font + 2; /* skip encoding and glyph size */
              }
              font += pgm_read_byte(font + 1);
            }
          }
          else
          {
            uint16_t e;
            font += _u8g2_start_pos_unicode;
            const uint8_t* unicode_lookup_table = font;

            /* issue 596: search for the glyph start in the unicode lookup table */
            do
            {
              font += u8g2_font_get_word(unicode_lookup_table, 0);
              e = u8g2_font_get_word(unicode_lookup_table, 2);
              unicode_lookup_table += 4;
            } while (e < _encoding);

            for (;;)
            {
              e = u8g2_font_get_word(font, 0);

              if (e == 0)
                break;

              if (e == _encoding)
              {
                glyph_data = font + 3; /* skip encoding and glyph size */
                break;
              }
              font += pgm_read_byte(font + 2);
            }
          }

          if (glyph_data)
          {
            // u8g2_font_decode_glyph
            _u8g2_decode_ptr = glyph_data;
            _u8g2_decode_bit_pos = 0;

            _u8g2_char_width = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_char_width);
            _u8g2_char_height = u8g2_font_decode_get_unsigned_bits(_u8g2_bits_per_char_height);
            _u8g2_char_x = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_char_x);
            _u8g2_char_y = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_char_y);
            _u8g2_delta_x = u8g2_font_decode_get_signed_bits(_u8g2_bits_per_delta_x);

            if (_u8g2_char_width > 0)
            {
              if (wrap && ((curr_x + (textsize_x * _u8g2_char_width) - 1) > _max_text_x))
              {
                curr_x = _min_text_x;
                curr_y += (int16_t)textsize_y * _u8g2_max_char_height;
              }
            }

            int16_t x1_tmp = curr_x + ((int16_t)_u8g2_char_x * textsize_x),
                    y1_tmp = curr_y - (((int16_t)_u8g2_char_height + _u8g2_char_y) * textsize_y),
                    x2 = x1_tmp + ((int16_t)_u8g2_char_width * textsize_x) - 1,
                    y2 = y1_tmp + ((int16_t)_u8g2_char_height * textsize_y) - 1;
            if (x1_tmp < minx)
            {
              minx = x1_tmp;
            }
            if (y1_tmp < miny)
            {
              miny = y1_tmp;
            }
            if (x2 > maxx)
            {
              maxx = x2;
            }
            if (y2 > maxy)
            {
              maxy = y2;
            }
            curr_x += (int16_t)textsize_x * _u8g2_delta_x;
          }
        }
      }
    }
    else  // glcdfont
    {
      if (c == '\n')
      {                            // Newline?
        curr_x = _min_text_x;      // Reset x to zero,
        curr_y += textsize_y * 8;  // advance y one line
                                   // min/max x/y unchaged -- that waits for next 'normal' character
      }
      else if (c != '\r')  // Normal char; ignore carriage returns
      {
        if (wrap && ((curr_x + (textsize_x * 6) - 1) > _max_text_x))  // Off right?
        {
          curr_x = _min_text_x;      // Reset x to zero,
          curr_y += textsize_y * 8;  // advance y one line
        }
        int16_t x2 = curr_x + textsize_x * 6 - 1;  // Lower-right pixel of char
        int16_t y2 = curr_y + textsize_y * 8 - 1;
        if (x2 > maxx)
        {
          maxx = x2;  // Track max x, y
        }
        if (y2 > maxy)
        {
          maxy = y2;
        }
        if (curr_x < minx)
        {
          minx = curr_x;  // Track min x, y
        }
        if (curr_y < miny)
        {
          miny = curr_y;
        }
        curr_x += textsize_x * 6;  // Advance x one char
      }
    }
  }

  if (maxx >= minx)
  {
    x1 = minx;
    w = maxx - minx + 1;
  }
  if (maxy >= miny)
  {
    y1 = miny;
    h = maxy - miny + 1;
  }
}

/**************************************************************************/
/*!
  @brief  Invert the display (ideally using built-in hardware command)
  @param  i   True if you want to invert, false to make 'normal'
*/
/**************************************************************************/
void Arduino_GFX::invertDisplay(bool)
{
  // Do nothing, must be subclassed if supported by hardware
}

/**************************************************************************/
/*!
  @brief  Turn on display after turned off
*/
/**************************************************************************/
void Arduino_GFX::displayOn()
{
}

/**************************************************************************/
/*!
  @brief  Turn off display
*/
/**************************************************************************/
void Arduino_GFX::displayOff()
{
}

void Arduino_GFX::drawBitmapToFramebuffer(
    const uint16_t* from_bitmap,
    int16_t bitmap_w,
    int16_t bitmap_h,
    uint16_t* framebuffer,
    int16_t x,
    int16_t y,
    int16_t framebuffer_w,
    int16_t framebuffer_h)
{
#if CONFIG_IDF_TARGET_ESP32P4
  bool can_use_ppa = true;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  int16_t max_X = framebuffer_w - 1;
  int16_t max_Y = framebuffer_h - 1;
  int16_t x_skip = 0;

  if ((y + bitmap_h - 1) > max_Y)
  {
    bitmap_h -= (y + bitmap_h - 1) - max_Y;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (y < 0)
  {
    from_bitmap -= y * bitmap_w;
    bitmap_h += y;
    y = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if ((x + bitmap_w - 1) > max_X)
  {
    x_skip = (x + bitmap_w - 1) - max_X;
    bitmap_w -= x_skip;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (x < 0)
  {
    from_bitmap -= x;
    x_skip -= x;
    bitmap_w += x;
    x = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled && can_use_ppa)
  {
    ppaRotate(from_bitmap, bitmap_w, bitmap_h, x, y, _framebuffer, WIDTH, HEIGHT, PPA_SRM_ROTATION_ANGLE_0);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    uint16_t* row = framebuffer;
    row += y * framebuffer_w;  // shift framebuffer to y offset
    row += x;                  // shift framebuffer to x offset
    if (((framebuffer_w & 1) == 0) && ((x_skip & 1) == 0) && ((bitmap_w & 1) == 0))
    {
      uint32_t* row2 = (uint32_t*)row;
      uint32_t* from_bitmap2 = (uint32_t*)from_bitmap;
      int16_t framebuffer_w2 = framebuffer_w >> 1;
      int16_t xskip2 = x_skip >> 1;
      int16_t w2 = bitmap_w >> 1;

      int16_t j = bitmap_h;
      while (j--)
      {
        for (int16_t i = 0; i < w2; ++i)
        {
          row2[i] = *from_bitmap2++;
        }
        from_bitmap2 += xskip2;
        row2 += framebuffer_w2;
      }
    }
    else
    {
      int16_t j = bitmap_h;
      while (j--)
      {
        for (int i = 0; i < bitmap_w; ++i)
        {
          row[i] = *from_bitmap++;
        }
        from_bitmap += x_skip;
        row += framebuffer_w;
      }
    }
  }
}

void Arduino_GFX::drawBitmapToFramebufferRotate1(
    const uint16_t* from_bitmap,
    int16_t bitmap_w,
    int16_t bitmap_h,
    uint16_t* framebuffer,
    int16_t x,
    int16_t y,
    int16_t framebuffer_w,
    int16_t framebuffer_h)
{
#if CONFIG_IDF_TARGET_ESP32P4
  bool can_use_ppa = true;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

  int16_t max_X = framebuffer_w - 1;
  int16_t max_Y = framebuffer_h - 1;
  int16_t x_skip = 0;

  if ((y + bitmap_h - 1) > max_Y)
  {
    bitmap_h -= (y + bitmap_h - 1) - max_Y;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (y < 0)
  {
    from_bitmap -= y * bitmap_w;
    bitmap_h += y;
    y = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if ((x + bitmap_w - 1) > max_X)
  {
    x_skip = (x + bitmap_w - 1) - max_X;
    bitmap_w -= x_skip;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (x < 0)
  {
    from_bitmap -= x;
    x_skip -= x;
    bitmap_w += x;
    x = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled && can_use_ppa)
  {
    uint16_t target_x = WIDTH - y - bitmap_h;
    uint16_t target_y = x;

    ppaRotate(from_bitmap, bitmap_w, bitmap_h, target_x, target_y, _framebuffer, WIDTH, HEIGHT, PPA_SRM_ROTATION_ANGLE_270);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    uint16_t* p;
    int16_t i;
    for (int16_t j = 0; j < bitmap_h; j++)
    {
      p = framebuffer;
      p += (x * framebuffer_h);          // shift framebuffer to y offset
      p += (framebuffer_h - y - j - 1);  // shift framebuffer to x offset

      i = bitmap_w;
      while (i--)
      {
        *p = *from_bitmap++;
        p += framebuffer_h;
      }
      from_bitmap += x_skip;
    }
  }
}

void Arduino_GFX::drawBitmapToFramebufferRotate2(
    const uint16_t* from_bitmap,
    int16_t bitmap_w,
    int16_t bitmap_h,
    uint16_t* framebuffer,
    int16_t x,
    int16_t y,
    int16_t framebuffer_w,
    int16_t framebuffer_h)
{
#if CONFIG_IDF_TARGET_ESP32P4
  bool can_use_ppa = true;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

  int16_t max_X = framebuffer_w - 1;
  int16_t max_Y = framebuffer_h - 1;
  int16_t x_skip = 0;

  if ((y + bitmap_h - 1) > max_Y)
  {
    bitmap_h -= (y + bitmap_h - 1) - max_Y;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (y < 0)
  {
    from_bitmap -= y * bitmap_w;
    bitmap_h += y;
    y = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if ((x + bitmap_w - 1) > max_X)
  {
    x_skip = (x + bitmap_w - 1) - max_X;
    bitmap_w -= x_skip;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (x < 0)
  {
    from_bitmap -= x;
    x_skip -= x;
    bitmap_w += x;
    x = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled && can_use_ppa)
  {
    uint16_t target_x = WIDTH - x - bitmap_w;
    uint16_t target_y = HEIGHT - y - bitmap_h;

    ppaRotate(from_bitmap, bitmap_w, bitmap_h, target_x, target_y, _framebuffer, WIDTH, HEIGHT, PPA_SRM_ROTATION_ANGLE_180);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    uint16_t* row = framebuffer;
    row += (max_Y - y) * framebuffer_w;   // shift framebuffer to y offset
    row += framebuffer_w - x - bitmap_w;  // shift framebuffer to x offset
    int16_t i;
    int16_t j = bitmap_h;
    while (j--)
    {
      i = bitmap_w;
      while (i--)
      {
        row[i] = *from_bitmap++;
      }
      from_bitmap += x_skip;
      row -= framebuffer_w;
    }
  }
}

void Arduino_GFX::drawBitmapToFramebufferRotate3(
    const uint16_t* from_bitmap,
    int16_t bitmap_w,
    int16_t bitmap_h,
    uint16_t* framebuffer,
    int16_t x,
    int16_t y,
    int16_t framebuffer_w,
    int16_t framebuffer_h)
{
#if CONFIG_IDF_TARGET_ESP32P4
  bool can_use_ppa = true;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

  int16_t max_X = framebuffer_w - 1;
  int16_t max_Y = framebuffer_h - 1;
  int16_t x_skip = 0;

  if ((y + bitmap_h - 1) > max_Y)
  {
    bitmap_h -= (y + bitmap_h - 1) - max_Y;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (y < 0)
  {
    from_bitmap -= y * bitmap_w;
    bitmap_h += y;
    y = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if ((x + bitmap_w - 1) > max_X)
  {
    x_skip = (x + bitmap_w - 1) - max_X;
    bitmap_w -= x_skip;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  if (x < 0)
  {
    from_bitmap -= x;
    x_skip -= x;
    bitmap_w += x;
    x = 0;
#if CONFIG_IDF_TARGET_ESP32P4
    can_use_ppa = false;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

#if CONFIG_IDF_TARGET_ESP32P4
  if (_ppa_enabled && can_use_ppa)
  {
    uint16_t target_x = y;
    uint16_t target_y = HEIGHT - x - bitmap_w;

    ppaRotate(from_bitmap, bitmap_w, bitmap_h, target_x, target_y, _framebuffer, WIDTH, HEIGHT, PPA_SRM_ROTATION_ANGLE_90);
  }
  else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  {
    uint16_t* p;
    int16_t i;
    for (int16_t j = 0; j < bitmap_h; j++)
    {
      p = framebuffer;
      p += ((max_X - x) * framebuffer_h);  // shift framebuffer to y offset
      p += y + j;                          // shift framebuffer to x offset

      i = bitmap_w;
      while (i--)
      {
        *p = *from_bitmap++;
        p -= framebuffer_h;
      }
      from_bitmap += x_skip;
    }
  }
}
