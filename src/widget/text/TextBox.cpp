#pragma GCC optimize("O3")
#include "TextBox.h"

#define H_PADDINGS (_h_padding * 2 - 2)

namespace pixeler
{
  TextBox::TextBox(uint16_t widget_ID, TypeID type_ID) : Label(widget_ID, type_ID)
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
  }

  TextBox* TextBox::clone(uint16_t id) const
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

  void TextBox::setType(FieldType type)
  {
    _type = type;
    _is_changed = true;
  }

  TextBox::FieldType TextBox::getType() const
  {
    return _type;
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

  void TextBox::addChar(char ch)
  {
    _text += ch;
    _text_len = calcRealStrLen(_text);
    _is_changed = true;
  }

  String TextBox::getSubStr(const String& str, uint16_t start, uint16_t length) const
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
      uint16_t pix_num = calcTextPixels(first_char_pos);

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

  void TextBox::onDraw()
  {
    if (!_is_changed)
      return;

    _is_changed = false;
    clear();

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextColor(_text_color);

    const uint16_t txt_y_pos = calcYStrOffset();
    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

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
      if (_type != TYPE_PASSWORD)
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
