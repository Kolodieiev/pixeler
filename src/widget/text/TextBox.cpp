#pragma GCC optimize("O3")

#include "TextBox.h"

#define H_PADDINGS (_h_padding * 2 - 2)

static const char STR_DEFAULT_TEXT_HINT[] = "Введіть текст";
static const char STR_DEFAULT_PASS_HINT[] = "Введіть пароль";

namespace pixeler
{
  TextBox::TextBox(const uint16_t widget_ID, const TypeID type_ID) : Label(widget_ID, type_ID)
  {
    _text_gravity = GRAVITY_CENTER;
    _back_color = COLOR_WHITE;
    _text_color = COLOR_BLACK;
  }

  void TextBox::copyTo(IWidget* widget) const
  {
    Label::copyTo(widget);

    TextBox* clone = static_cast<TextBox*>(widget);
    clone->_type = _type;
    clone->_text_gravity = GRAVITY_CENTER;
    clone->_hint = _hint;
    clone->_hint_color = _hint_color;
    clone->_is_hint_enabled = _is_hint_enabled;
  }

  TextBox* TextBox::clone(const uint16_t id) const
  {
    try
    {
      TextBox* clone = new TextBox(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void TextBox::setType(const FieldType type)
  {
    _type = type;
    _is_changed = true;
  }

  TextBox::FieldType TextBox::getType() const
  {
    return _type;
  }

  void TextBox::setHint(const char* hint)
  {
    _hint = hint ? hint : "";
    if (_text.isEmpty() && _is_hint_enabled)
      _is_changed = true;
  }

  void TextBox::setHint(const String& hint)
  {
    _hint = hint;
    if (_text.isEmpty() && _is_hint_enabled)
      _is_changed = true;
  }

  const String& TextBox::getHint() const
  {
    return _hint;
  }

  void TextBox::setHintColor(const uint16_t color)
  {
    _hint_color = color;
    if (_text.isEmpty() && _is_hint_enabled)
      _is_changed = true;
  }

  uint16_t TextBox::getHintColor() const
  {
    return _hint_color;
  }

  void TextBox::setHintEnabled(const bool state)
  {
    if (_is_hint_enabled == state)
      return;

    _is_hint_enabled = state;
    if (_text.isEmpty())
      _is_changed = true;
  }

  bool TextBox::isHintEnabled() const
  {
    return _is_hint_enabled;
  }

  bool TextBox::removeLastChar()
  {
    if (_text.isEmpty())
      return false;

    _text = getSubStr(_text, 0, _text_len - 1);
    _text_len = calcRealStrLen(_text);

    _is_changed = true;
    return true;
  }

  void TextBox::addChars(const char* ch)
  {
    _text += ch;
    _text_len = calcRealStrLen(_text);
    _is_changed = true;
  }

  void TextBox::addChar(const char ch)
  {
    _text += ch;
    _text_len = calcRealStrLen(_text);
    _is_changed = true;
  }

  String TextBox::getSubStr(const String& str, const uint16_t start, const uint16_t length) const
  {
    if (length == 0 || str.isEmpty())
      return emptyString;

    const uint8_t* buf = reinterpret_cast<const uint8_t*>(str.c_str());
    const uint16_t start_byte = charPosToByte(buf, start);

    if (start_byte >= str.length())
      return emptyString;

    const uint16_t end_byte = start_byte + charPosToByte(buf + start_byte, length);

    return str.substring(start_byte, end_byte);
  }

  uint16_t TextBox::getFitStr(String& ret_str) const
  {
    uint16_t first_char_pos{1};

    while (first_char_pos < _text_len)
    {
      const uint16_t pix_num = calcTextPixels(first_char_pos);

      if (pix_num < _width - H_PADDINGS)
      {
        const uint8_t* ch_str_p8 = reinterpret_cast<const uint8_t*>(_text.c_str());
        const uint16_t byte_pos = charPosToByte(ch_str_p8, first_char_pos);
        ret_str = _text.substring(byte_pos);
        return pix_num;
      }

      ++first_char_pos;
    }

    return 0;
  }

  void TextBox::drawHint(const uint16_t abs_x, const uint16_t abs_y, const uint16_t txt_y_pos) const
  {
    const char* hint_cstr = nullptr;
    if (!_hint.isEmpty())
      hint_cstr = _hint.c_str();
    else if (_type == TYPE_PASSWORD)
      hint_cstr = STR_DEFAULT_PASS_HINT;
    else
      hint_cstr = STR_DEFAULT_TEXT_HINT;

    int16_t x1{0};
    int16_t y1{0};
    uint16_t str_pix_num{0u};
    uint16_t h{0u};
    _display.calcTextBounds(hint_cstr, 0, 0, x1, y1, str_pix_num, h);

    const int32_t avail_w = static_cast<int32_t>(_width) - H_PADDINGS;
    if (avail_w <= 0)
      return;

    if (str_pix_num < static_cast<uint16_t>(avail_w))
    {
      const uint16_t txt_x_pos = calcXStrOffset(str_pix_num);
      _display.setTextColor(_hint_color);
      _display.setCursor(abs_x + txt_x_pos, abs_y + txt_y_pos - _y_char_offset);
      _display.print(hint_cstr);
      return;
    }

    String fit_hint(hint_cstr);
    const size_t total_bytes = fit_hint.length();
    char* buf = const_cast<char*>(fit_hint.c_str());

    size_t byte_pos{0u};
    size_t last_valid_byte{0u};
    uint16_t fit_pix_num{0u};

    while (byte_pos < total_bytes)
    {
      const uint8_t b = static_cast<uint8_t>(buf[byte_pos]);
      uint8_t char_len{1u};

      if ((b & 0x80u) == 0x00u)
        char_len = 1u;
      else if ((b & 0xE0u) == 0xC0u)
        char_len = 2u;
      else if ((b & 0xF0u) == 0xE0u)
        char_len = 3u;
      else if ((b & 0xF8u) == 0xF0u)
        char_len = 4u;

      if (byte_pos + char_len > total_bytes)
        char_len = static_cast<uint8_t>(total_bytes - byte_pos);

      byte_pos += char_len;

      const char saved = buf[byte_pos];
      buf[byte_pos] = '\0';

      uint16_t w{0u};
      _display.calcTextBounds(buf, 0, 0, x1, y1, w, h);

      buf[byte_pos] = saved;

      if (w < static_cast<uint16_t>(avail_w))
      {
        fit_pix_num = w;
        last_valid_byte = byte_pos;
      }
      else
      {
        break;
      }
    }

    if (last_valid_byte == 0u)
      return;

    buf[last_valid_byte] = '\0';
    const uint16_t txt_x_pos = calcXStrOffset(fit_pix_num);
    _display.setTextColor(_hint_color);
    _display.setCursor(abs_x + txt_x_pos, abs_y + txt_y_pos - _y_char_offset);
    _display.print(buf);
  }

  void TextBox::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;
    clear();

    _display.setFont(_font);
    _display.setTextSize(_text_size);

    const uint16_t txt_y_pos = calcYStrOffset();
    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    if (_text.isEmpty())
    {
      if (_is_hint_enabled)
        drawHint(abs_x, abs_y, txt_y_pos);

      return;
    }

    _display.setTextColor(_text_color);

    uint16_t str_pix_num = 0;

    if (_type == TYPE_PASSWORD && !_text.isEmpty())
    {
      // Отримуємо ширину однієї зірочки у поточному шрифті
      int16_t x1, y1;
      uint16_t w, h;
      _display.calcTextBounds("*", 0, 0, x1, y1, w, h);

      str_pix_num = w * _text_len;
    }
    else
    {
      str_pix_num = calcTextPixels();
    }

    uint16_t txt_x_pos = 0;
    String draw_str;

    uint16_t visible_stars = 0;

    // Визначаємо зміщення та текст, який вміщується у віджет
    if (str_pix_num + H_PADDINGS < _width)
    {
      txt_x_pos = calcXStrOffset(str_pix_num);
      if (_type == TYPE_PASSWORD)
        visible_stars = _text_len;
      else
        draw_str = _text;
    }
    else
    {
      if (_type == TYPE_PASSWORD)
      {
        // Підраховуємо кількість зірочок, які вмістяться у віджет
        int16_t x1, y1;
        uint16_t star_w, h;
        _display.calcTextBounds("*", 0, 0, x1, y1, star_w, h);

        const uint16_t avail_width = _width - H_PADDINGS;
        visible_stars = (star_w > 0) ? (avail_width / star_w) : 0;

        // Встановлюємо ширину підрядка для правильного центрування
        const uint16_t sub_str_pix_num = visible_stars * star_w;
        txt_x_pos = calcXStrOffset(sub_str_pix_num);
      }
      else
      {
        const uint16_t sub_str_pix_num = getFitStr(draw_str);
        txt_x_pos = calcXStrOffset(sub_str_pix_num);
      }
    }

    _display.setCursor(abs_x + txt_x_pos, abs_y + txt_y_pos - _y_char_offset);

    // Малюємо зірочки по одній, не виділяючи пам'ять під новий String
    if (_type == TYPE_PASSWORD && visible_stars > 0)
    {
      for (uint16_t i = 0; i < visible_stars; ++i)
        _display.print("*");
    }
    else if (!draw_str.isEmpty())
    {
      _display.print(draw_str.c_str());
    }
  }
}  // namespace pixeler
