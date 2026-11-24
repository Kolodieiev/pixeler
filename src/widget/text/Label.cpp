#include "Label.h"

constexpr static uint16_t RESERVE_PIX_WIDTH = 7;
constexpr static uint16_t RESERVE_PIX_HEIGHT = 4;

namespace pixeler
{
  Label::Label(uint16_t widget_ID, TypeID type_ID) : IWidget(widget_ID, type_ID)
  {
    updateHeight();
  }

  void Label::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    Label* clone = static_cast<Label*>(widget);
    clone->_is_multiline = _is_multiline;
    clone->_text_size = _text_size;
    clone->_text_color = _text_color;
    clone->_font = _font;
    clone->_char_hgt = _char_hgt;
    clone->_y_char_offset = _y_char_offset;
    clone->_h_padding = _h_padding;
    clone->_text_gravity = _text_gravity;
    clone->_text_alignment = _text_alignment;
    clone->_has_autoscroll = _has_autoscroll;
    clone->_temp_has_autoscroll = _temp_has_autoscroll;
    clone->_has_full_autoscroll = _has_full_autoscroll;
    clone->_has_autoscroll_in_focus = _has_autoscroll_in_focus;
    clone->_temp_has_autoscroll_in_focus = _temp_has_autoscroll_in_focus;
    clone->_temp_width = _temp_width;
    clone->_autoscroll_update_delay = _autoscroll_update_delay;
    clone->_is_reverse_autoscroll = _is_reverse_autoscroll;

    clone->setText(_text);
    clone->_back_img = _back_img;
  }

  Label* Label::clone(uint16_t id) const
  {
    try
    {
      Label* clone = new Label(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void Label::initWidthToFit(uint16_t add_width_value)
  {
    _width = calcTextPixels();
    _width += RESERVE_PIX_WIDTH + add_width_value + _h_padding * 2;

    _is_changed = true;
  }

  void Label::updateWidthToFit(uint16_t add_width_value)
  {
    _temp_width = calcTextPixels();
    _temp_width += RESERVE_PIX_WIDTH + add_width_value + _h_padding * 2;

    if (_temp_width == _width)
      _temp_width = 0;

    _is_changed = true;
  }

  void Label::setText(const String& text)
  {
    try
    {
      _text = text;
      _text_len = calcRealStrLen(_text);
      _has_autoscroll = _temp_has_autoscroll;
      _has_autoscroll_in_focus = _temp_has_autoscroll_in_focus;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      _text = "err";
      _text_len = 0;
    }

    _first_draw_char_pos = 0;
    _is_changed = true;
  }

  const String& Label::getText() const
  {
    return _text;
  }

  void Label::setTextSize(uint8_t size)
  {
    if (size == 0)
      size = 1;

    _text_size = size;
    _is_changed = true;
    updateHeight();
  }

  void Label::setTextColor(uint16_t textColor)
  {
    _text_color = textColor;
    _is_changed = true;
  }

  bool Label::hasAutoscrollInFocus() const
  {
    return _temp_has_autoscroll_in_focus;
  }

  uint16_t Label::getCharHgt() const
  {
    return _char_hgt;
  }

  void Label::setBackImg(Image* back_img)
  {
    _back_img = back_img;
    _is_changed = true;
  }

  Image* Label::getBackImg() const
  {
    return _back_img;
  }

  void Label::setMultiline(bool state)
  {
    _is_multiline = state;
    _is_changed = true;
  }

  bool Label::isMultiline() const
  {
    return _is_multiline;
  }

  void Label::setAutoscrollDelay(uint16_t delay)
  {
    _autoscroll_update_delay = delay;
    _is_changed = true;
  }

  uint16_t Label::getAutoscrollDelay() const
  {
    return _autoscroll_update_delay;
  }

  void Label::setAutoscroll(bool state)
  {
    _has_autoscroll = state;
    _temp_has_autoscroll = state;

    if (!state)
      _first_draw_char_pos = 0;

    _is_changed = true;
  }

  bool Label::hasAutoscroll() const
  {
    return _temp_has_autoscroll;
  }

  void Label::setAutoscrollInFocus(bool state)
  {
    _has_autoscroll_in_focus = state;
    _temp_has_autoscroll_in_focus = state;

    if (!state && _has_autoscroll)
      _first_draw_char_pos = 0;

    _is_changed = true;
  }

  void Label::setFont(const uint8_t* font)
  {
    if (!font)
      font = font_unifont;

    _font = font;
    _is_changed = true;

    updateHeight();
  }

  const uint8_t* Label::getFont() const
  {
    return _font;
  }

  void Label::setGravity(Gravity gravity)
  {
    _text_gravity = gravity;
    _is_changed = true;
  }

  void Label::setAlign(Alignment alignment)
  {
    _text_alignment = alignment;
    _is_changed = true;
  }

  void Label::setHPadding(uint8_t padding)
  {
    _h_padding = padding;
    _is_changed = true;
  }

  uint16_t Label::getLen() const
  {
    return _text_len;
  }

  void Label::setFullAutoscroll(bool state)
  {
    _is_changed = true;
    _first_draw_char_pos = 0;
    _has_full_autoscroll = state;
    _is_reverse_autoscroll = false;
  }

  uint16_t Label::calcXStrOffset(uint16_t str_pix_num) const
  {
    if (_width <= str_pix_num + _h_padding * 2 + RESERVE_PIX_WIDTH)
      return 1;

    if (_has_autoscroll)
      return _h_padding + 1;

    switch (_text_alignment)
    {
      case ALIGN_START:
        return _h_padding + 1;
      case ALIGN_CENTER:
        return (_width - str_pix_num) / 2;
      case ALIGN_END:
        return _width - str_pix_num - _h_padding - RESERVE_PIX_WIDTH / 2;
      default:
        return 1;
    }
  }

  uint16_t Label::calcYStrOffset() const
  {
    if (_height > _char_hgt)
    {
      switch (_text_gravity)
      {
        case GRAVITY_TOP:
          return 1;
        case GRAVITY_CENTER:
          return (_height - _char_hgt) / 2;
        case GRAVITY_BOTTOM:
          return _height - _char_hgt - 1;
      }
    }

    return 0;
  }

  uint32_t Label::calcRealStrLen(const String& str) const
  {
    if (str.isEmpty())
      return 0;

    uint32_t length{0};

    for (const char c : str)
    {
      if ((c & 0xC0) != 0x80)
        ++length;
    }
    return length;
  }

  uint16_t Label::calcTextPixels(uint16_t char_pos) const
  {
    if (_text.isEmpty())
      return 0;

    int16_t x1{0};
    int16_t y1{0};
    uint16_t w{0};
    uint16_t h{0};

    uint32_t byte_pos{0};
    if (char_pos > 0)
    {
      const uint8_t* ch_str_p8 = reinterpret_cast<const uint8_t*>(_text.c_str());
      byte_pos = charPosToByte(ch_str_p8, char_pos);
    }

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.calcTextBounds(_text.c_str() + byte_pos, 0, 0, x1, y1, w, h);

    return w;
  }

  uint16_t Label::charPosToByte(const uint8_t* buf, uint16_t char_pos) const
  {
    if (char_pos == 0)
      return 0;

    uint16_t char_num = 0;
    uint16_t i = 0;

    while (char_num < char_pos && buf[i] != '\0')
    {
      if ((buf[i] & 0x80) == 0x00)
        ++i;
      else if ((buf[i] & 0xE0) == 0xC0)
        i += 2;
      else if ((buf[i] & 0xF0) == 0xE0)
        i += 3;
      else if ((buf[i] & 0xF8) == 0xF0)
        i += 4;
      else
        ++i;

      ++char_num;
    }

    return i;
  }

  void Label::updateHeight()
  {
    int16_t x1;
    uint16_t w;
    _display.setTextSize(_text_size);
    _display.setFont(_font);
    _display.calcTextBounds("Йg", 0, 0, x1, _y_char_offset, w, _char_hgt);

    if (_char_hgt + RESERVE_PIX_HEIGHT >= _height)
      _height = _char_hgt + RESERVE_PIX_HEIGHT + 2;
  }

  uint32_t Label::utf8ToUnicode(const uint8_t* buf, uint16_t& byte_pos, uint16_t remaining) const
  {
    uint32_t codepoint = 0;

    if ((buf[byte_pos] & 0x80) == 0x00)
    {
      // 1-byte sequence
      codepoint = buf[byte_pos];
      ++byte_pos;
    }
    else if ((buf[byte_pos] & 0xE0) == 0xC0 && remaining > 1)
    {
      // 2-byte sequence
      codepoint = (buf[byte_pos] & 0x1F) << 6;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F);
      ++byte_pos;
    }
    else if ((buf[byte_pos] & 0xF0) == 0xE0 && remaining > 2)
    {
      // 3-byte sequence
      codepoint = (buf[byte_pos] & 0x0F) << 12;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F) << 6;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F);
      ++byte_pos;
    }
    else if ((buf[byte_pos] & 0xF8) == 0xF0 && remaining > 3)
    {
      // 4-byte sequence
      codepoint = (buf[byte_pos] & 0x07) << 18;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F) << 12;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F) << 6;
      ++byte_pos;
      codepoint |= (buf[byte_pos] & 0x3F);
      ++byte_pos;
    }
    else
      ++byte_pos;

    return codepoint;
  }

  uint16_t Label::getFitStr(String& ret_str, uint16_t start_pos) const
  {
    ret_str = "";

    const uint16_t avail_width = _h_padding * 2 + 2;
    if (start_pos >= _text_len || _width <= avail_width)
      return 0;

    const uint16_t max_text_width = _width - avail_width;
    const uint16_t byte_len = _text.length();

    char* ch_str = const_cast<char*>(_text.c_str());
    const uint8_t* ch_str_p8 = reinterpret_cast<const uint8_t*>(ch_str);

    uint16_t cur_byte_pos = charPosToByte(ch_str_p8, start_pos);
    const uint16_t start_byte_pos = cur_byte_pos;
    uint16_t last_valid_byte_pos = start_byte_pos;

    uint16_t pix_sum{0};
    int16_t x1, y1;
    uint16_t w, h;

    while (cur_byte_pos < byte_len)
    {
      const uint32_t code = utf8ToUnicode(ch_str_p8, cur_byte_pos, byte_len - cur_byte_pos);
      if (code == 0)
        continue;

      const char temp_char = ch_str[cur_byte_pos];
      ch_str[cur_byte_pos] = '\0';

      _display.calcTextBounds(ch_str + start_byte_pos, 0, 0, x1, y1, w, h);

      ch_str[cur_byte_pos] = temp_char;

      if (w < max_text_width)
      {
        pix_sum = w;
        last_valid_byte_pos = cur_byte_pos;
      }
      else
      {
        break;
      }
    }

    if (last_valid_byte_pos > start_byte_pos)
      ret_str = _text.substring(start_byte_pos, last_valid_byte_pos);

    return pix_sum;
  }

  void Label::onDraw()
  {
    if (!_is_changed)
    {
      if ((!_has_autoscroll && !(_has_autoscroll_in_focus && _has_focus)) || _visibility == INVISIBLE)
        return;
      else if ((millis() - _last_autoscroll_ts) < _autoscroll_update_delay)
        return;
    }

    _is_changed = false;

    if (_visibility == INVISIBLE)
    {
      hide();
      return;
    }

    if (_has_autoscroll_in_focus && !_has_focus)
      _first_draw_char_pos = 0;

    if (_temp_width > 0)
    {
      if (_temp_width < _width)
        hide();

      _width = _temp_width;
      _temp_width = 0;
    }

    clear();

    if (_back_img)
    {
      _back_img->setParent(this);
      _back_img->drawForced();
    }

    if (_text.isEmpty())
      return;

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextColor(_text_color);

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    if (_is_multiline)
    {
      _first_draw_char_pos = 0;
      _has_autoscroll = false;
      _has_autoscroll_in_focus = false;

      _display.setTextWrap(true);
      _display.setTextBound(abs_x + 1, abs_y + 1, _width - 1, _height - 1);
      _display.setCursor(abs_x + 1, abs_y + _char_hgt + 2);
      _display.print(_text.c_str());
      _display.resetTextBound();
      _display.setTextWrap(false);

      return;
    }

    const uint16_t txt_y_pos = calcYStrOffset();
    const uint16_t str_pix_num = calcTextPixels();

    if (str_pix_num + _h_padding * 2 < _width + 2)
    {
      _has_autoscroll = false;
      _has_autoscroll_in_focus = false;
      const uint16_t txt_x_pos = calcXStrOffset(str_pix_num);
      _display.setCursor(abs_x + txt_x_pos, abs_y + txt_y_pos - _y_char_offset);
      _display.print(_text.c_str());

      return;
    }

    String sub_str;
    const uint16_t sub_str_pix_num = getFitStr(sub_str, _first_draw_char_pos);
    const uint16_t txt_x_pos = calcXStrOffset(sub_str_pix_num);

    if (_has_autoscroll || (_has_autoscroll_in_focus && _has_focus))
    {
      if ((millis() - _last_autoscroll_ts) > _autoscroll_update_delay)
      {
        if (_has_full_autoscroll)
        {
          if (_first_draw_char_pos == _text_len - 1 || sub_str_pix_num == 0)
            _first_draw_char_pos = 0;
          else
            ++_first_draw_char_pos;
        }
        else if (!_is_reverse_autoscroll)
        {
          if (!_text.endsWith(sub_str))
            ++_first_draw_char_pos;
          else
            _is_reverse_autoscroll = true;
        }
        else
        {
          if (_first_draw_char_pos > 0)
            --_first_draw_char_pos;
          else
            _is_reverse_autoscroll = false;
        }

        _last_autoscroll_ts = millis();
      }
    }

    _display.setCursor(abs_x + txt_x_pos, abs_y + txt_y_pos - _y_char_offset);
    _display.print(sub_str.c_str());
  }
}  // namespace pixeler
