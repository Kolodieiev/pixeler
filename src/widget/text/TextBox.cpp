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

  uint16_t TextBox::getFitStr(String& ret_str) const
  {
    uint16_t first_char_pos{1};

    const char* ch_str = _text.c_str();

    while (first_char_pos < _text_len - 1)
    {
      uint16_t pix_num = calcTextPixels(first_char_pos);

      if (pix_num < _width - H_PADDINGS)
      {
        ret_str = getSubStr(ch_str, first_char_pos, calcRealStrLen(ch_str) - 1);
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

    _display.setFont(_font_ptr);
    _display.setTextSize(_text_size);
    _display.setTextColor(_text_color);

    uint16_t txtYPos = calcYStrOffset();
    uint16_t str_pix_num = calcTextPixels();

    uint16_t x_offset{0};
    uint16_t y_offset{0};

    if (_parent)
    {
      x_offset = _parent->getXPos();
      y_offset = _parent->getYPos();
    }

    if (str_pix_num + H_PADDINGS < _width)
    {
      uint16_t txt_x_pos = calcXStrOffset(str_pix_num);

      if (_type == TYPE_TEXT)
      {
        _display.setCursor(_x_pos + x_offset + txt_x_pos, _y_pos + y_offset + txtYPos - _y_char_offset);
        _display.print(_text.c_str());
      }
      else  // pwd
      {
        uint32_t txt_len = calcRealStrLen(_text);

        String pwd_str = "";

        for (uint32_t i = 0; i < txt_len; ++i)
          pwd_str += "*";

        _display.setCursor(_x_pos + x_offset + txt_x_pos, _y_pos + y_offset + txtYPos - _y_char_offset);
        _display.print(pwd_str.c_str());
      }
    }
    else
    {
      String sub_str;
      uint16_t sub_str_pix_num = getFitStr(sub_str);
      uint16_t txt_x_pos = calcXStrOffset(sub_str_pix_num);

      if (_type == TYPE_TEXT)
      {
        _display.setCursor(_x_pos + x_offset + txt_x_pos, _y_pos + y_offset + txtYPos - _y_char_offset);
        _display.print(sub_str.c_str());
      }
      else  // pwd
      {
        uint32_t txt_len = calcRealStrLen(sub_str);

        String pwd_str = "";

        for (uint32_t i = 0; i < txt_len; ++i)
          pwd_str += "*";

        _display.setCursor(_x_pos + x_offset + txt_x_pos, _y_pos + y_offset + txtYPos - _y_char_offset);
        _display.print(pwd_str.c_str());
      }
    }
  }
}  // namespace pixeler
