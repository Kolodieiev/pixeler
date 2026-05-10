#pragma GCC optimize("O3")
/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#pragma once

#include <Print.h>

#if CONFIG_IDF_TARGET_ESP32P4
#include <driver/ppa.h>
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

#include "Arduino_DataBus.h"

#ifndef DEGTORAD
#define DEGTORAD 0.017453292519943295769236907684886F
#endif

#include "U8g2/u8g2.h"

// Default color definitions
#define COLOR_BLACK 0x0000
#define COLOR_NAVY 0x000F
#define COLOR_DARKGREEN 0x03E0
#define COLOR_DARKCYAN 0x03EF
#define COLOR_MAROON 0x7800
#define COLOR_PURPLE 0x780F
#define COLOR_OLIVE 0x7BE0
#define COLOR_LIGHTGREY 0xD69A
#define COLOR_LIME 0xA7E0
#define COLOR_DARKGREY 0x0020
#define COLOR_GREY 0xAD55
#define COLOR_BLUE 0x001F
#define COLOR_GREEN 0x07E0
#define COLOR_CYAN 0x07FF
#define COLOR_RED 0xF800
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE 0xFFFF
#define COLOR_ORANGE 0xFDA0
#define COLOR_GREENYELLOW 0xB7E0
#define COLOR_PINK 0xFE19
#define COLOR_BROWN 0x9A60
#define COLOR_GOLD 0xFEA0
#define COLOR_SILVER 0xC618
#define COLOR_SKYBLUE 0x867D
#define COLOR_VIOLET 0x915C

#define COLOR_TRANSPARENT 0xF81F  // Колір rgb(255, 0, 255)

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_sbyte
#define pgm_read_sbyte(addr) (*(const signed char*)(addr))
#endif

#define pgm_read_pointer(addr) ((void*)pgm_read_dword(addr))

#ifndef _swap_uint8_t
#define _swap_uint8_t(a, b) \
  {                         \
    uint8_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

#ifndef _diff
#define _diff(a, b) ((a > b) ? (a - b) : (b - a))
#endif

#ifndef _ordered_in_range
#define _ordered_in_range(v, a, b) ((a <= v) && (v <= b))
#endif

#ifndef _in_range
#define _in_range(v, a, b) ((a > b) ? _ordered_in_range(v, b, a) : _ordered_in_range(v, a, b))
#endif

/// A generic graphics superclass that can handle all sorts of drawing. At a minimum you can subclass and provide drawPixel(). At a maximum you can do a ton of overriding to optimize. Used for any/all Adafruit displays!
class Arduino_GFX : public Print
{
public:
  Arduino_GFX(int16_t w, int16_t h);  // Constructor
  virtual ~Arduino_GFX();

  // This MUST be defined by the subclass:
  virtual bool begin(int32_t speed = GFX_NOT_DEFINED) = 0;
  virtual void writePixelPreclipped(int16_t x, int16_t y, uint16_t color);

  // TRANSACTION API / CORE DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void startWrite();
  virtual void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void endWrite(void);

  // CONTROL API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void setRotation(uint8_t r);
  virtual void invertDisplay(bool i);
  virtual void displayOn();
  virtual void displayOff();

  // BASIC DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  // It's good to implement those, even if using transaction API
  void writePixel(int16_t x, int16_t y, uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillScreen(uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  void getTextBounds(const char* string, int16_t x, int16_t y, int16_t& x1, int16_t& y1, uint16_t& w, uint16_t& h);
  void setTextSize(uint8_t s);
  void setTextSize(uint8_t sx, uint8_t sy);
  void setTextSize(uint8_t sx, uint8_t sy, uint8_t pixel_margin);

  void setFont(const uint8_t* font);
  void setUTF8Print(bool isEnable);
  uint16_t u8g2_font_get_word(const uint8_t* font, uint8_t offset);
  uint8_t u8g2_font_decode_get_unsigned_bits(uint8_t cnt);
  int8_t u8g2_font_decode_get_signed_bits(uint8_t cnt);
  void u8g2_font_decode_len(uint8_t len, uint8_t is_foreground, uint16_t color, uint16_t bg);
  virtual void flushMainBuff();
  virtual void duplicateMainBuff();
  virtual void flushSecondBuff();

  // adopt from LovyanGFX
  void drawEllipse(int16_t x, int16_t y, int16_t rx, int16_t ry, uint16_t color);
  void writeEllipseHelper(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t cornername, uint16_t color);
  void fillEllipse(int16_t x, int16_t y, int16_t rx, int16_t ry, uint16_t color);
  void writeFillEllipseHelper(int32_t x, int32_t y, int32_t rx, int32_t ry, uint8_t cornername, int16_t delta, uint16_t color);
  void drawArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color);
  void fillArc(int16_t x, int16_t y, int16_t r1, int16_t r2, float start, float end, uint16_t color);
  void writeFillArcHelper(int16_t cx, int16_t cy, int16_t oradius, int16_t iradius, float start, float end, uint16_t color);

  virtual void writeSlashLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);
  virtual void draw16bitRGBBitmapWithTranColor(int16_t x, int16_t y, const uint16_t* bitmap, uint16_t transparent_color, int16_t w, int16_t h);
  virtual void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg);

  /**
   * @brief Має ефект, тільки якщо підтримується на МК.
   * Встановлює стан активності модуля апаратного прискорення для операцій заповнення кольором
   * та копіювання зображень в буфер кадру.
   * НЕ ЕФЕКТИВНЕ ДЛЯ МАЛИХ БЛОКІВ.
   * @param state
   */
  void setPPAState(bool state);

  /**
   * @brief Повертає стан активності PPA модуля.
   *
   * @return true - Якщо PPA увімкнено.
   * @return false - Інакше.
   */
  bool isPPAEnabled() const;

  /**********************************************************************/
  /*!
    @brief  Set text cursor location
    @param  x    X coordinate in pixels
    @param  y    Y coordinate in pixels
  */
  /**********************************************************************/
  void setCursor(int16_t x, int16_t y)
  {
    cursor_x = x;
    cursor_y = y;
  }

  /**********************************************************************/
  /*!
    @brief  Set text bound for printing
    @param  x    X coordinate in pixels
    @param  y    Y coordinate in pixels
  */
  void setTextBound(int16_t x, int16_t y, int16_t w, int16_t h);

  /**
   * @brief Прибирає поле, яке встановлює кордони для виводу текста.
   *
   */
  void resetTextBound();

  /**********************************************************************/
  /*!
    @brief   Set text font color with transparant background
    @param   c   16-bit 5-6-5 Color to draw text with
    @note    For 'transparent' background, background and foreground
             are set to same color rather than using a separate flag.
  */
  /**********************************************************************/
  void setTextColor(uint16_t c)
  {
    textcolor = textbgcolor = c;
  }

  /**********************************************************************/
  /*!
    @brief   Set text font color with custom background color
    @param   c   16-bit 5-6-5 Color to draw text with
    @param   bg  16-bit 5-6-5 Color to draw background/fill with
  */
  /**********************************************************************/
  void setTextColor(uint16_t c, uint16_t bg)
  {
    textcolor = c;
    textbgcolor = bg;
  }

  /**********************************************************************/
  /*!
  @brief  Set whether text that is too long for the screen width should
          automatically wrap around to the next line (else clip right).
  @param  w  true for wrapping, false for clipping
  */
  /**********************************************************************/
  void setTextWrap(bool w)
  {
    wrap = w;
  }

  virtual size_t write(uint8_t);

  /************************************************************************/
  /*!
    @brief      Get width of the display, accounting for current rotation
    @returns    Width in pixels
  */
  /************************************************************************/
  int16_t width(void) const
  {
    return _width;
  };

  /************************************************************************/
  /*!
    @brief      Get height of the display, accounting for current rotation
    @returns    Height in pixels
  */
  /************************************************************************/
  int16_t height(void) const
  {
    return _height;
  }

  /************************************************************************/
  /*!
    @brief      Get rotation setting for display
    @returns    0 thru 3 corresponding to 4 cardinal rotations
  */
  /************************************************************************/
  uint8_t getRotation(void) const
  {
    return _rotation;
  }

  // get current cursor position (get rotation safe maximum values,
  // using: width() for x, height() for y)
  /************************************************************************/
  /*!
    @brief  Get text cursor X location
    @returns    X coordinate in pixels
  */
  /************************************************************************/
  int16_t getCursorX(void) const
  {
    return cursor_x;
  }

  /************************************************************************/
  /*!
    @brief      Get text cursor Y location
    @returns    Y coordinate in pixels
  */
  /************************************************************************/
  int16_t getCursorY(void) const
  {
    return cursor_y;
  };

  /*!
    @brief   Given 8-bit red, green and blue values, return a 'packed'
             16-bit color value in '565' RGB format (5 bits red, 6 bits
             green, 5 bits blue). This is just a mathematical operation,
             no hardware is touched.
    @param   red    8-bit red brightnesss (0 = off, 255 = max).
    @param   green  8-bit green brightnesss (0 = off, 255 = max).
    @param   blue   8-bit blue brightnesss (0 = off, 255 = max).
    @return  'Packed' 16-bit color value (565 format).
  */
  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue)
  {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  }

#if CONFIG_IDF_TARGET_ESP32P4
  // void pushPPAImg();// TODO

#endif  // #if CONFIG_IDF_TARGET_ESP32P4

  void drawBitmapToFramebufferRotate1(
      const uint16_t* from_bitmap,
      int16_t bitmap_w,
      int16_t bitmap_h,
      uint16_t* framebuffer,
      int16_t x,
      int16_t y,
      int16_t framebuffer_w,
      int16_t framebuffer_h);

  void drawBitmapToFramebufferRotate2(
      const uint16_t* from_bitmap,
      int16_t bitmap_w,
      int16_t bitmap_h,
      uint16_t* framebuffer,
      int16_t x,
      int16_t y,
      int16_t framebuffer_w,
      int16_t framebuffer_h);

  void drawBitmapToFramebufferRotate3(
      const uint16_t* from_bitmap,
      int16_t bitmap_w,
      int16_t bitmap_h,
      uint16_t* framebuffer,
      int16_t x,
      int16_t y,
      int16_t framebuffer_w,
      int16_t framebuffer_h);

protected:
  void drawBitmapToFramebuffer(
      const uint16_t* from_bitmap,
      int16_t bitmap_w,
      int16_t bitmap_h,
      uint16_t* framebuffer,
      int16_t x,
      int16_t y,
      int16_t framebuffer_w,
      int16_t framebuffer_h);

#if CONFIG_IDF_TARGET_ESP32P4
  void ppaFill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void ppaRotate(const void* buff_from,
                 uint16_t buff_f_w,
                 uint16_t buff_f_h,
                 uint16_t x,
                 uint16_t y,
                 void* buff_to,
                 uint16_t buff_t_w,
                 uint16_t buff_t_h,
                 ppa_srm_rotation_angle_t angle);

  uint32_t color16RGBTo24RGB(uint16_t color565);

#endif  // #if CONFIG_IDF_TARGET_ESP32P4

protected:
#if CONFIG_IDF_TARGET_ESP32P4
  ppa_client_handle_t _ppa_fill{nullptr};
  ppa_client_handle_t _ppa_srm{nullptr};

  ppa_fill_oper_config_t _fill_oper;
  ppa_srm_oper_config_t _srm_oper;
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

  const uint8_t* u8g2Font{nullptr};
  const uint8_t* _u8g2_decode_ptr{nullptr};
  uint16_t* _framebuffer{nullptr};

  const uint32_t FRAMEBUFF_SIZE;
  const uint16_t WIDTH;   ///< This is the 'raw' display width - never changes
  const uint16_t HEIGHT;  ///< This is the 'raw' display height - never changes

  uint16_t _encoding;
  uint16_t _u8g2_start_pos_upper_A;
  uint16_t _u8g2_start_pos_lower_a;
  uint16_t _u8g2_start_pos_unicode;
  int16_t _u8g2_target_x;
  int16_t _u8g2_target_y;

  int16_t _width;   ///< Display width as modified by current rotation
  int16_t _height;  ///< Display height as modified by current rotation
  int16_t _max_x;   ///< x zero base bound (_width - 1)
  int16_t _max_y;   ///< y zero base bound (_height - 1)
  int16_t _min_text_x;
  int16_t _min_text_y;
  int16_t _max_text_x;
  int16_t _max_text_y;
  int16_t cursor_x;  ///< x location to start print()ing text
  int16_t cursor_y;  ///< y location to start print()ing text

  uint16_t textcolor;    ///< 16-bit background color for print()
  uint16_t textbgcolor;  ///< 16-bit text color for print()

  uint8_t _utf8_state = 0;
  uint8_t _u8g2_glyph_cnt;
  uint8_t _u8g2_bits_per_0;
  uint8_t _u8g2_bits_per_1;
  uint8_t _u8g2_bits_per_char_width;
  uint8_t _u8g2_bits_per_char_height;
  uint8_t _u8g2_bits_per_char_x;
  uint8_t _u8g2_bits_per_char_y;
  uint8_t _u8g2_bits_per_delta_x;
  int8_t _u8g2_max_char_width;
  int8_t _u8g2_max_char_height;
  uint8_t _u8g2_first_char;
  uint8_t _u8g2_char_width;
  uint8_t _u8g2_char_height;
  int8_t _u8g2_char_x;
  int8_t _u8g2_char_y;
  int8_t _u8g2_delta_x;
  int8_t _u8g2_dx;
  int8_t _u8g2_dy;

  uint8_t textsize_x;         ///< Desired magnification in X-axis of text to print()
  uint8_t textsize_y;         ///< Desired magnification in Y-axis of text to print()
  uint8_t text_pixel_margin;  ///< Margin for each text pixel
  uint8_t _rotation;          ///< Display rotation (0 thru 3)

  uint8_t _u8g2_decode_bit_pos;

  bool _enableUTF8Print = false;
  bool wrap;  ///< If set, 'wrap' text at right edge of display

#if CONFIG_IDF_TARGET_ESP32P4
  bool _ppa_enabled{false};
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
};
