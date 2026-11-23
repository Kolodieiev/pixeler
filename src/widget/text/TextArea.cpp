#pragma GCC optimize("O3")

#include "TextArea.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static constexpr uint8_t CACHE_EMPTY{0xFFu};
static constexpr uint8_t MAX_CACHED_WIDTH{254u};
static constexpr size_t UTF8_CACHE_MASK{63u};
static constexpr uint32_t UTF8_HASH_MULT{2654435761u};
static constexpr uint8_t TAB_SPACES{2u};
static constexpr uint16_t CURSOR_WIDTH{2u};
static constexpr uint16_t MIN_BLINK_DELAY{50u};
static constexpr size_t GAP_MIN_RESERVE{64u};
static constexpr size_t GAP_MAX_RESERVE{4096u};
static constexpr size_t DRAW_CHUNK_SIZE{64u};
static constexpr uint16_t MIN_NL_HIGHLIGHT_W{4u};
static constexpr uint8_t MIN_GUTTER_DIGITS{2u};
static constexpr uint16_t GUTTER_HPAD{3u};

namespace pixeler
{
  TextArea::TextArea(const uint16_t widget_ID, const TypeID type_ID) : IWidget(widget_ID, type_ID)
  {
    _back_color = COLOR_WHITE;
    updateFontMetrics();
    _last_blink_ts = millis();
  }

  TextArea::~TextArea()
  {
    clearUndoHistory();
    if (_undo_buf)
    {
      free(_undo_buf);
      _undo_buf = nullptr;
    }

    if (_buf)
    {
      free(_buf);
      _buf = nullptr;
    }

    clearClipboard();
  }

  void TextArea::copyTo(IWidget* widget) const
  {
    IWidget::copyTo(widget);

    TextArea* clone = static_cast<TextArea*>(widget);
    clone->_font = _font;
    clone->_text_size = _text_size;
    clone->_text_color = _text_color;
    clone->_cursor_color = _cursor_color;
    clone->_hint_color = _hint_color;
    clone->_sel_back_color = _sel_back_color;
    clone->_sel_text_color = _sel_text_color;
    clone->_line_num_back_color = _line_num_back_color;
    clone->_line_num_text_color = _line_num_text_color;
    clone->_line_num_border_color = _line_num_border_color;
    clone->_show_line_numbers = _show_line_numbers;
    clone->_cursor_blink_delay = _cursor_blink_delay;
    clone->_h_padding = _h_padding;
    clone->_v_padding = _v_padding;
    clone->_hint = _hint;

    clone->setMaxUndoSteps(_undo_cap);
    clone->_undo_mem_limit = _undo_mem_limit;

    clone->updateFontMetrics();
    clone->_gap_start = 0u;
    clone->_gap_end = clone->_buf_cap;
    clone->_cursor_pos = 0u;
    clone->_sel_anchor = 0u;
    clone->_view_top_byte = 0u;
    clone->_char_len = 0u;
    clone->_total_lines = 1u;
    clone->_view_top_line_num = 1u;
    clone->_is_selecting = false;
    clone->clearClipboard();

    const size_t total_bytes = getByteLen();
    if (total_bytes > 0u && clone->ensureGap(total_bytes))
    {
      copyRange(0u, total_bytes, clone->_buf);
      clone->_gap_start = total_bytes;
      clone->_char_len = _char_len;
      clone->_total_lines = _total_lines;
      clone->_cursor_pos = _cursor_pos;
      clone->_sel_anchor = _sel_anchor;
      clone->_is_selecting = _is_selecting;
      clone->_view_top_byte = _view_top_byte;
      clone->_view_top_line_num = _view_top_line_num;
    }

    if (_clip_buf && _clip_len > 0u)
    {
      char* new_clip = static_cast<char*>(psramInit() ? ps_malloc(_clip_len) : malloc(_clip_len));
      if (new_clip)
      {
        memcpy(new_clip, _clip_buf, _clip_len);
        clone->_clip_buf = new_clip;
        clone->_clip_len = _clip_len;
      }
      else
      {
        log_e("Помилка виділення пам'яті для копіювання буфера обміну TextArea (%u байт)", static_cast<uint32_t>(_clip_len));
      }
    }

    clone->_is_changed = true;
  }

  TextArea* TextArea::clone(const uint16_t id) const
  {
    try
    {
      TextArea* clone = new TextArea(id);
      copyTo(clone);
      return clone;
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  char TextArea::byteAt(const size_t pos) const
  {
    if (pos < _gap_start)
      return _buf[pos];

    return _buf[pos + (_gap_end - _gap_start)];
  }

  void TextArea::copyRange(const size_t from, const size_t len, char* dst) const
  {
    if (!_buf || len == 0u)
      return;

    const size_t end = from + len;
    const size_t gap_size = _gap_end - _gap_start;

    if (end <= _gap_start)
    {
      memcpy(dst, _buf + from, len);
    }
    else if (from >= _gap_start)
    {
      memcpy(dst, _buf + from + gap_size, len);
    }
    else
    {
      const size_t first_len = _gap_start - from;
      memcpy(dst, _buf + from, first_len);
      memcpy(dst + first_len, _buf + _gap_end, len - first_len);
    }
  }

  void TextArea::moveGap(const size_t pos) const
  {
    if (!_buf || pos == _gap_start)
      return;

    if (pos < _gap_start)
    {
      const size_t count = _gap_start - pos;
      memmove(_buf + _gap_end - count, _buf + pos, count);
      _gap_start -= count;
      _gap_end -= count;
    }
    else
    {
      const size_t count = pos - _gap_start;
      memmove(_buf + _gap_start, _buf + _gap_end, count);
      _gap_start += count;
      _gap_end += count;
    }
  }

  bool TextArea::ensureGap(const size_t needed)
  {
    const size_t cur_gap = _gap_end - _gap_start;
    if (cur_gap >= needed && _buf)
      return true;

    const size_t cur_len = _buf_cap - cur_gap;
    if (needed > SIZE_MAX - cur_len - 1u)
    {
      log_e("Переповнення розміру буфера TextArea");
      return false;
    }

    size_t extra = cur_len / 4u;
    if (extra < GAP_MIN_RESERVE)
      extra = GAP_MIN_RESERVE;
    else if (extra > GAP_MAX_RESERVE)
      extra = GAP_MAX_RESERVE;

    size_t new_cap = cur_len + needed;
    if (extra <= SIZE_MAX - new_cap - 1u)
      new_cap += extra;

    const size_t alloc_size = new_cap + 1u;
    char* new_buf = nullptr;

    if (psramInit())
      new_buf = static_cast<char*>(_buf ? ps_realloc(_buf, alloc_size) : ps_malloc(alloc_size));
    else
      new_buf = static_cast<char*>(_buf ? realloc(_buf, alloc_size) : malloc(alloc_size));

    if (!new_buf)
    {
      log_e("Помилка виділення пам'яті для TextArea (%u байт)", static_cast<uint32_t>(alloc_size));
      return false;
    }

    const size_t tail_len = _buf_cap - _gap_end;
    const size_t new_gap_end = new_cap - tail_len;
    if (tail_len > 0u)
      memmove(new_buf + new_gap_end, new_buf + _gap_end, tail_len);

    _buf = new_buf;
    _buf_cap = new_cap;
    _gap_end = new_gap_end;
    return true;
  }

  uint8_t TextArea::utf8LenAt(const size_t pos) const
  {
    const size_t len = getByteLen();
    if (pos >= len)
      return 0u;

    const uint8_t b = static_cast<uint8_t>(byteAt(pos));
    uint8_t char_len{1u};

    if ((b & 0x80u) == 0x00u)
      char_len = 1u;
    else if ((b & 0xE0u) == 0xC0u)
      char_len = 2u;
    else if ((b & 0xF0u) == 0xE0u)
      char_len = 3u;
    else if ((b & 0xF8u) == 0xF0u)
      char_len = 4u;

    const size_t remaining = len - pos;
    return (char_len <= remaining) ? char_len : static_cast<uint8_t>(remaining);
  }

  size_t TextArea::prevUtf8Pos(const size_t pos) const
  {
    if (pos == 0u)
      return 0u;

    size_t prev = pos - 1u;
    uint8_t steps{1u};

    while (prev > 0u && steps < 4u && (static_cast<uint8_t>(byteAt(prev)) & 0xC0u) == 0x80u)
    {
      --prev;
      ++steps;
    }

    return prev;
  }

  uint32_t TextArea::decodeUtf8At(const size_t pos, uint8_t& out_len) const
  {
    out_len = utf8LenAt(pos);
    if (out_len == 0u)
      return 0u;

    const uint8_t b0 = static_cast<uint8_t>(byteAt(pos));

    if (out_len == 1u)
      return b0;

    if (out_len == 2u)
    {
      const uint8_t b1 = static_cast<uint8_t>(byteAt(pos + 1u));
      return (static_cast<uint32_t>(b0 & 0x1Fu) << 6u) | (b1 & 0x3Fu);
    }

    if (out_len == 3u)
    {
      const uint8_t b1 = static_cast<uint8_t>(byteAt(pos + 1u));
      const uint8_t b2 = static_cast<uint8_t>(byteAt(pos + 2u));
      return (static_cast<uint32_t>(b0 & 0x0Fu) << 12u) |
          (static_cast<uint32_t>(b1 & 0x3Fu) << 6u) |
          (b2 & 0x3Fu);
    }

    const uint8_t b1 = static_cast<uint8_t>(byteAt(pos + 1u));
    const uint8_t b2 = static_cast<uint8_t>(byteAt(pos + 2u));
    const uint8_t b3 = static_cast<uint8_t>(byteAt(pos + 3u));
    return (static_cast<uint32_t>(b0 & 0x07u) << 18u) |
        (static_cast<uint32_t>(b1 & 0x3Fu) << 12u) |
        (static_cast<uint32_t>(b2 & 0x3Fu) << 6u) |
        (b3 & 0x3Fu);
  }

  uint32_t TextArea::countNewlines(const size_t from, const size_t to) const
  {
    if (!_buf || from >= to)
      return 0u;

    uint32_t count{0u};
    const size_t gap_size = _gap_end - _gap_start;

    auto countSlice = [&count](const char* ptr, size_t len)
    {
      while (len > 0u)
      {
        const void* found = memchr(ptr, '\n', len);
        if (!found)
          break;

        ++count;
        const char* next = static_cast<const char*>(found) + 1;
        len -= static_cast<size_t>(next - ptr);
        ptr = next;
      }
    };

    if (to <= _gap_start)
    {
      countSlice(_buf + from, to - from);
    }
    else if (from >= _gap_start)
    {
      countSlice(_buf + from + gap_size, to - from);
    }
    else
    {
      countSlice(_buf + from, _gap_start - from);
      countSlice(_buf + _gap_end, to - _gap_start);
    }

    return count;
  }

  bool TextArea::ensureUndoBuf()
  {
    if (_undo_cap == 0u)
      return false;

    if (_undo_buf)
      return true;

    const size_t bytes = static_cast<size_t>(_undo_cap) * sizeof(UndoStep);
    _undo_buf = static_cast<UndoStep*>(psramInit() ? ps_malloc(bytes) : malloc(bytes));
    if (!_undo_buf)
    {
      log_e("Помилка виділення пам'яті для історії undo TextArea (%u байт)", static_cast<uint32_t>(bytes));
      return false;
    }

    memset(_undo_buf, 0, bytes);
    return true;
  }

  void TextArea::freeUndoStep(UndoStep& step)
  {
    if (step.removed_buf)
    {
      free(step.removed_buf);
      step.removed_buf = nullptr;
      _undo_used_bytes = (_undo_used_bytes > step.removed_len) ? (_undo_used_bytes - step.removed_len) : 0u;
    }

    step.removed_len = 0u;
  }

  bool TextArea::evictOldestUndo()
  {
    if (!_undo_buf || _undo_count == 0u)
      return false;

    freeUndoStep(_undo_buf[_undo_head]);
    _undo_head = static_cast<uint8_t>((_undo_head + 1u) % _undo_cap);
    --_undo_count;
    return true;
  }

  void TextArea::breakUndoGroup()
  {
    if (!_undo_buf || _undo_count == 0u)
      return;

    const uint8_t idx = static_cast<uint8_t>((_undo_head + _undo_count - 1u) % _undo_cap);
    _undo_buf[idx].is_insert_group = false;
  }

  void TextArea::recordUndoStep(const size_t pos, const size_t removed_len, const size_t inserted_len, const size_t prev_cursor, const size_t prev_sel_anchor, const bool prev_is_selecting, const uint32_t prev_char_len, const uint32_t prev_total_lines, const bool can_coalesce)
  {
    if (!ensureUndoBuf())
      return;

    if (can_coalesce && removed_len == 0u && _undo_count > 0u)
    {
      const uint8_t last_idx = static_cast<uint8_t>((_undo_head + _undo_count - 1u) % _undo_cap);
      UndoStep& last = _undo_buf[last_idx];
      if (last.is_insert_group && last.removed_len == 0u && (last.pos + last.inserted_len == pos))
      {
        last.inserted_len += inserted_len;
        return;
      }
    }

    char* saved_buf = nullptr;
    if (removed_len > 0u)
    {
      if (removed_len > _undo_mem_limit)
      {
        // Пропуск кроку порушив би ланцюг дельт, тому очищуємо попередню історію
        clearUndoHistory();
        return;
      }

      while (_undo_count > 0u && (_undo_used_bytes + removed_len > _undo_mem_limit))
        evictOldestUndo();

      saved_buf = static_cast<char*>(psramInit() ? ps_malloc(removed_len) : malloc(removed_len));
      while (!saved_buf && _undo_count > 0u)
      {
        evictOldestUndo();
        saved_buf = static_cast<char*>(psramInit() ? ps_malloc(removed_len) : malloc(removed_len));
      }

      if (!saved_buf)
      {
        log_e("Помилка виділення пам'яті для кроку undo TextArea (%u байт)", static_cast<uint32_t>(removed_len));
        clearUndoHistory();
        return;
      }

      copyRange(pos, removed_len, saved_buf);
      _undo_used_bytes += removed_len;
    }

    if (_undo_count == _undo_cap)
      evictOldestUndo();

    const uint8_t write_idx = static_cast<uint8_t>((_undo_head + _undo_count) % _undo_cap);
    UndoStep& step = _undo_buf[write_idx];
    step.removed_buf = saved_buf;
    step.pos = pos;
    step.removed_len = removed_len;
    step.inserted_len = inserted_len;
    step.prev_cursor = prev_cursor;
    step.prev_sel_anchor = prev_sel_anchor;
    step.prev_char_len = prev_char_len;
    step.prev_total_lines = prev_total_lines;
    step.prev_is_selecting = prev_is_selecting;
    step.is_insert_group = can_coalesce && (removed_len == 0u);
    ++_undo_count;
  }

  void TextArea::invalidateGlyphCache()
  {
    memset(_ascii_w, CACHE_EMPTY, sizeof(_ascii_w));
    memset(_utf8_cp, 0, sizeof(_utf8_cp));
    memset(_utf8_w, 0, sizeof(_utf8_w));
  }

  void TextArea::updateFontMetrics()
  {
    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextWrap(false);
    _display.resetTextBound();

    int16_t x1{0};
    uint16_t w1{0u};
    _display.calcTextBounds("Йg", 0, 0, x1, _y_char_offset, w1, _char_hgt);

    int16_t tx{0};
    int16_t ty{0};
    uint16_t tw{0u};
    uint16_t h1{0u};
    uint16_t h2{0u};
    _display.calcTextBounds("A", 0, 0, tx, ty, tw, h1);
    _display.calcTextBounds("A\nA", 0, 0, tx, ty, tw, h2);

    if (h2 > h1)
      _line_step = h2 - h1;
    else
      _line_step = _display.getFontHeight(_font, _text_size);

    if (_line_step < _char_hgt)
      _line_step = _char_hgt;

    if (_line_step == 0u)
      _line_step = 8u * _text_size;

    if (_char_hgt == 0u)
      _char_hgt = _line_step;

    _display.calcTextBounds("HH", 0, 0, tx, ty, _ref_pair_w, h1);

    invalidateGlyphCache();

    uint8_t max_digit_w{0u};
    for (char d{'0'}; d <= '9'; ++d)
    {
      const uint8_t dw = getCharWidth(static_cast<uint32_t>(d), &d, 1u);
      if (dw > max_digit_w)
        max_digit_w = dw;
    }

    _digit_w = (max_digit_w > 0u) ? max_digit_w : static_cast<uint8_t>(6u * _text_size);

    _cached_width = 0u;
    _cached_height = 0u;
    _cached_gutter_w = 0u;
    _is_changed = true;
  }

  uint16_t TextArea::calcGutterWidth() const
  {
    if (!_show_line_numbers)
      return 0u;

    uint32_t lines = _total_lines;
    uint8_t digits{1u};
    while (lines >= 10u)
    {
      lines /= 10u;
      ++digits;
    }

    if (digits < MIN_GUTTER_DIGITS)
      digits = MIN_GUTTER_DIGITS;

    const uint16_t gutter_w = static_cast<uint16_t>(digits) * _digit_w + GUTTER_HPAD * 2u + 1u;
    const uint16_t border = _has_border ? 1u : 0u;
    const uint16_t min_text_w = static_cast<uint16_t>(_digit_w) * 2u + static_cast<uint16_t>(_h_padding) * 2u + border * 2u;

    if (_width <= gutter_w + min_text_w)
      return 0u;

    return gutter_w;
  }

  TextArea::ContentRect TextArea::getContentRect() const
  {
    const uint16_t border = _has_border ? 1u : 0u;
    const uint16_t gutter_w = calcGutterWidth();
    const uint16_t left_offset = border + gutter_w + _h_padding;
    const uint16_t right_offset = border + _h_padding;
    const uint16_t pad_y = border + _v_padding;

    const uint16_t abs_x = getXPos();
    const uint16_t abs_y = getYPos();

    ContentRect rect{};
    rect.x = abs_x + left_offset;
    rect.y = abs_y + pad_y;
    rect.w = (_width > left_offset + right_offset) ? (_width - left_offset - right_offset) : 0u;
    rect.h = (_height > pad_y * 2u) ? (_height - pad_y * 2u) : 0u;
    return rect;
  }

  void TextArea::syncLayoutMetrics()
  {
    const uint16_t gutter_w = calcGutterWidth();
    if (_cached_width == _width && _cached_height == _height && _cached_gutter_w == gutter_w)
      return;

    _cached_width = _width;
    _cached_height = _height;
    _cached_gutter_w = gutter_w;

    const ContentRect rect = getContentRect();
    const uint16_t lines = (_line_step > 0u) ? (rect.h / _line_step) : 1u;
    _visible_lines = (lines > 0u) ? lines : 1u;

    if (rect.w > 0u && getByteLen() > 0u)
    {
      VisualLine top_line{};
      findVisualLineAtPos(_view_top_byte, rect.w, top_line);
      setViewTop(top_line.start);
      ensureCursorVisible();
    }
  }

  uint8_t TextArea::getCharWidth(const uint32_t cp, const char* utf8_bytes, const uint8_t utf8_len) const
  {
    if (cp == '\r' || cp == '\n')
      return 0u;

    if (cp == '\t')
    {
      const uint16_t tab_w = static_cast<uint16_t>(getCharWidth(' ', " ", 1u)) * TAB_SPACES;
      return (tab_w > MAX_CACHED_WIDTH) ? MAX_CACHED_WIDTH : static_cast<uint8_t>(tab_w);
    }

    if (cp < 0x20u)
      return 0u;

    if (cp <= 0x7Eu)
    {
      const size_t idx = static_cast<size_t>(cp - 0x20u);
      if (_ascii_w[idx] != CACHE_EMPTY)
        return _ascii_w[idx];

      // Обчислення точного xAdvance символу через різницю ширини "H<ch>H" та "HH"
      char probe[4]{'H', static_cast<char>(cp), 'H', '\0'};
      int16_t px{0};
      int16_t py{0};
      uint16_t pw{0u};
      uint16_t ph{0u};

      _display.setFont(_font);
      _display.setTextSize(_text_size);
      _display.setTextWrap(false);
      _display.calcTextBounds(probe, 0, 0, px, py, pw, ph);

      const uint16_t diff = (pw > _ref_pair_w) ? (pw - _ref_pair_w) : 0u;
      const uint8_t adv = (diff > MAX_CACHED_WIDTH) ? MAX_CACHED_WIDTH : static_cast<uint8_t>(diff);
      _ascii_w[idx] = adv;
      return adv;
    }

    const size_t slot = (cp * UTF8_HASH_MULT) & UTF8_CACHE_MASK;
    if (_utf8_cp[slot] == cp)
      return _utf8_w[slot];

    char probe[8];
    probe[0] = 'H';
    const uint8_t copy_len = (utf8_len <= 4u) ? utf8_len : 4u;
    memcpy(probe + 1, utf8_bytes, copy_len);
    probe[1u + copy_len] = 'H';
    probe[2u + copy_len] = '\0';

    int16_t px{0};
    int16_t py{0};
    uint16_t pw{0u};
    uint16_t ph{0u};

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextWrap(false);
    _display.calcTextBounds(probe, 0, 0, px, py, pw, ph);

    const uint16_t diff = (pw > _ref_pair_w) ? (pw - _ref_pair_w) : 0u;
    const uint8_t adv = (diff > MAX_CACHED_WIDTH) ? MAX_CACHED_WIDTH : static_cast<uint8_t>(diff);
    _utf8_cp[slot] = cp;
    _utf8_w[slot] = adv;
    return adv;
  }

  uint8_t TextArea::getCharWidthAt(const size_t pos, uint8_t& out_len) const
  {
    const uint32_t cp = decodeUtf8At(pos, out_len);
    if (out_len == 0u)
      return 0u;

    if (out_len == 1u)
    {
      const char ch = static_cast<char>(cp);
      return getCharWidth(cp, &ch, 1u);
    }

    char utf8_bytes[4];
    for (uint8_t i{0u}; i < out_len; ++i)
      utf8_bytes[i] = byteAt(pos + i);

    return getCharWidth(cp, utf8_bytes, out_len);
  }

  uint16_t TextArea::measureRangeWidth(const size_t from, const size_t to) const
  {
    uint16_t width{0u};
    size_t pos = from;

    while (pos < to)
    {
      uint8_t clen{0u};
      width += getCharWidthAt(pos, clen);
      if (clen == 0u)
        break;

      pos += clen;
    }

    return width;
  }

  void TextArea::calcVisualLine(const size_t start_byte, const uint16_t max_w, VisualLine& out) const
  {
    const size_t len = getByteLen();
    out.start = start_byte;
    out.end = start_byte;
    out.ends_with_nl = false;

    if (start_byte >= len)
      return;

    uint16_t cur_w{0u};
    size_t pos = start_byte;

    while (pos < len)
    {
      const char c = byteAt(pos);
      if (c == '\n')
      {
        out.end = pos + 1u;
        out.ends_with_nl = true;
        return;
      }

      uint8_t clen{0u};
      const uint8_t cw = getCharWidthAt(pos, clen);
      if (clen == 0u)
        break;

      if (cur_w > 0u && max_w > 0u && cur_w + cw > max_w)
      {
        out.end = pos;
        return;
      }

      cur_w += cw;
      pos += clen;
    }

    out.end = pos;
  }

  bool TextArea::findPrevVisualLine(const size_t target_line_start, const uint16_t max_w, VisualLine& out) const
  {
    if (target_line_start == 0u)
      return false;

    size_t para_start = (byteAt(target_line_start - 1u) == '\n') ? (target_line_start - 1u) : target_line_start;
    while (para_start > 0u && byteAt(para_start - 1u) != '\n')
      --para_start;

    size_t pos = para_start;
    VisualLine cur{};

    while (pos < target_line_start)
    {
      calcVisualLine(pos, max_w, cur);
      if (cur.end >= target_line_start || cur.end <= pos)
        break;

      pos = cur.end;
    }

    out = cur;
    return true;
  }

  void TextArea::findVisualLineAtPos(const size_t pos, const uint16_t max_w, VisualLine& out) const
  {
    const size_t len = getByteLen();
    const size_t clamped = (pos > len) ? len : pos;

    size_t para_start = clamped;
    while (para_start > 0u && byteAt(para_start - 1u) != '\n')
      --para_start;

    size_t cur_pos = para_start;
    while (true)
    {
      calcVisualLine(cur_pos, max_w, out);
      if (lineOwnsCursor(out, clamped) || out.end <= cur_pos)
        return;

      cur_pos = out.end;
    }
  }

  bool TextArea::lineOwnsCursor(const VisualLine& line, const size_t pos) const
  {
    if (pos >= line.start && pos < line.end)
      return true;

    return pos == line.end && line.end == getByteLen() && !line.ends_with_nl;
  }

  size_t TextArea::lineCursorEnd(const VisualLine& line) const
  {
    if (line.ends_with_nl)
      return line.end - 1u;

    if (line.end == getByteLen())
      return line.end;

    return prevUtf8Pos(line.end);
  }

  size_t TextArea::closestPosAtX(const VisualLine& line, const uint16_t target_x) const
  {
    const size_t max_pos = lineCursorEnd(line);
    size_t pos = line.start;
    uint16_t cur_x{0u};

    while (pos < max_pos)
    {
      uint8_t clen{0u};
      const uint8_t cw = getCharWidthAt(pos, clen);
      if (clen == 0u)
        break;

      if (target_x <= cur_x + cw / 2u)
        return pos;

      cur_x += cw;
      pos += clen;
    }

    return max_pos;
  }

  void TextArea::setViewTop(const size_t new_top)
  {
    if (new_top == _view_top_byte)
      return;

    if (new_top == 0u)
    {
      _view_top_byte = 0u;
      _view_top_line_num = 1u;
      return;
    }

    if (new_top > _view_top_byte)
    {
      _view_top_line_num += countNewlines(_view_top_byte, new_top);
    }
    else
    {
      const uint32_t delta = countNewlines(new_top, _view_top_byte);
      _view_top_line_num = (_view_top_line_num > delta) ? (_view_top_line_num - delta) : 1u;
    }

    _view_top_byte = new_top;
  }

  void TextArea::recalcViewTopLineNum()
  {
    const size_t len = getByteLen();
    if (_view_top_byte > len)
      _view_top_byte = len;

    _view_top_line_num = 1u + countNewlines(0u, _view_top_byte);
  }

  void TextArea::getSelectionRange(size_t& out_start, size_t& out_end) const
  {
    if (!_is_selecting || _sel_anchor == _cursor_pos)
    {
      out_start = 0u;
      out_end = 0u;
      return;
    }

    if (_sel_anchor < _cursor_pos)
    {
      out_start = _sel_anchor;
      out_end = _cursor_pos;
    }
    else
    {
      out_start = _cursor_pos;
      out_end = _sel_anchor;
    }
  }

  bool TextArea::ensureCursorVisible()
  {
    const ContentRect rect = getContentRect();
    if (rect.w == 0u)
      return false;

    const size_t old_top = _view_top_byte;

    if (_cursor_pos < _view_top_byte)
    {
      VisualLine cur_line{};
      findVisualLineAtPos(_cursor_pos, rect.w, cur_line);
      setViewTop(cur_line.start);
      return _view_top_byte != old_top;
    }

    size_t pos = _view_top_byte;
    const size_t len = getByteLen();

    for (uint16_t i{0u}; i < _visible_lines; ++i)
    {
      VisualLine line{};
      calcVisualLine(pos, rect.w, line);

      if (lineOwnsCursor(line, _cursor_pos))
        return false;

      if (line.end >= len && !line.ends_with_nl)
        break;

      pos = line.end;
    }

    // Курсор нижче viewport: знаходимо рядок курсора і відступаємо вгору на (_visible_lines - 1)
    VisualLine line{};
    findVisualLineAtPos(_cursor_pos, rect.w, line);
    size_t new_top = line.start;

    for (uint16_t i{1u}; i < _visible_lines; ++i)
    {
      VisualLine prev{};
      if (!findPrevVisualLine(new_top, rect.w, prev))
        break;

      new_top = prev.start;
    }

    setViewTop(new_top);
    return _view_top_byte != old_top;
  }

  void TextArea::syncViewportAfterEdit(const bool force_recalc_top_line)
  {
    const size_t len = getByteLen();
    if (len == 0u)
    {
      _view_top_byte = 0u;
      _view_top_line_num = 1u;
      syncLayoutMetrics();
      return;
    }

    if (_view_top_byte > len)
    {
      _view_top_byte = len;
      recalcViewTopLineNum();
    }
    else if (force_recalc_top_line)
    {
      recalcViewTopLineNum();
    }

    syncLayoutMetrics();
    const ContentRect rect = getContentRect();
    if (rect.w == 0u)
      return;

    VisualLine top_line{};
    findVisualLineAtPos(_view_top_byte, rect.w, top_line);
    setViewTop(top_line.start);

    ensureCursorVisible();

    // Підтягуємо viewport вгору, якщо після видалення знизу залишилися порожні рядки
    uint16_t drawn_lines{0u};
    size_t pos = _view_top_byte;

    while (drawn_lines < _visible_lines)
    {
      VisualLine line{};
      calcVisualLine(pos, rect.w, line);
      ++drawn_lines;

      if (line.end >= len && !line.ends_with_nl)
        break;

      pos = line.end;
    }

    while (drawn_lines < _visible_lines && _view_top_byte > 0u)
    {
      VisualLine prev{};
      if (!findPrevVisualLine(_view_top_byte, rect.w, prev))
        break;

      setViewTop(prev.start);
      ++drawn_lines;
    }
  }

  void TextArea::updateCursorScreenCoords()
  {
    const ContentRect rect = getContentRect();
    _cursor_x = static_cast<int16_t>(rect.x);
    _cursor_y = static_cast<int16_t>(rect.y);

    if (rect.w == 0u || getByteLen() == 0u)
      return;

    size_t pos = _view_top_byte;
    const size_t len = getByteLen();

    for (uint16_t row{0u}; row < _visible_lines; ++row)
    {
      VisualLine line{};
      calcVisualLine(pos, rect.w, line);

      if (lineOwnsCursor(line, _cursor_pos))
      {
        const uint16_t offset_x = measureRangeWidth(line.start, _cursor_pos);
        _cursor_x = static_cast<int16_t>(rect.x + offset_x);
        _cursor_y = static_cast<int16_t>(rect.y + row * _line_step);
        return;
      }

      if (line.end >= len && !line.ends_with_nl)
        break;

      pos = line.end;
    }
  }

  void TextArea::finishCursorMove(const bool scrolled)
  {
    breakUndoGroup();
    _cursor_visible = true;
    _last_blink_ts = millis();
    updateCursorScreenCoords();

    if (scrolled || _is_selecting)
      _is_changed = true;
    else
      _is_cursor_dirty = true;
  }

  bool TextArea::pasteInternal(const char* text, const size_t byte_len, const bool can_coalesce)
  {
    if (!text)
      return false;

    if (byte_len == 0u)
    {
      if (hasSelection())
        return deleteSelected();

      return true;
    }

    size_t clean_len{0u};
    uint32_t added_chars{0u};
    uint32_t added_nl{0u};

    for (size_t i{0u}; i < byte_len; ++i)
    {
      const char c = text[i];
      if (c == '\r')
        continue;

      ++clean_len;
      if (c == '\n')
        ++added_nl;

      if ((static_cast<uint8_t>(c) & 0xC0u) != 0x80u)
        ++added_chars;
    }

    if (clean_len == 0u)
      return true;

    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);
    const size_t sel_len = sel_end - sel_start;

    const size_t extra_needed = (clean_len > sel_len) ? (clean_len - sel_len) : 0u;
    if (!ensureGap(extra_needed))
      return false;

    const size_t edit_pos = (sel_len > 0u) ? sel_start : _cursor_pos;
    recordUndoStep(edit_pos, sel_len, clean_len, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, can_coalesce && sel_len == 0u);

    if (sel_len > 0u)
    {
      uint32_t removed_chars{0u};
      uint32_t removed_nl{0u};
      for (size_t i{sel_start}; i < sel_end; ++i)
      {
        const char c = byteAt(i);
        if (c == '\n')
          ++removed_nl;

        if ((static_cast<uint8_t>(c) & 0xC0u) != 0x80u)
          ++removed_chars;
      }

      moveGap(sel_start);
      _gap_end += sel_len;
      _char_len = (_char_len > removed_chars) ? (_char_len - removed_chars) : 0u;
      _total_lines = (_total_lines > removed_nl) ? (_total_lines - removed_nl) : 1u;
      _cursor_pos = sel_start;
    }

    moveGap(_cursor_pos);

    char* dst = _buf + _gap_start;
    for (size_t i{0u}; i < byte_len; ++i)
    {
      const char c = text[i];
      if (c != '\r')
        *dst++ = c;
    }

    _gap_start += clean_len;
    _cursor_pos = _gap_start;
    _char_len += added_chars;
    _total_lines += added_nl;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(edit_pos < _view_top_byte);
    _is_changed = true;
    return true;
  }

  bool TextArea::paste()
  {
    if (!_clip_buf || _clip_len == 0u)
      return false;

    return pasteInternal(_clip_buf, _clip_len, false);
  }

  bool TextArea::paste(const char* text)
  {
    if (!text)
      return false;

    return pasteInternal(text, strlen(text), false);
  }

  bool TextArea::paste(const String& text)
  {
    return pasteInternal(text.c_str(), text.length(), false);
  }

  bool TextArea::paste(const char* text, const size_t byte_len)
  {
    return pasteInternal(text, byte_len, false);
  }

  bool TextArea::addChar(const char ch)
  {
    if (ch == '\0' || ch == '\r')
      return true;

    const bool can_coalesce = !hasSelection() && (ch != '\n');
    return pasteInternal(&ch, 1u, can_coalesce);
  }

  bool TextArea::addChar(const char* utf8_ch)
  {
    if (!utf8_ch)
      return false;

    const size_t len = strlen(utf8_ch);
    const bool can_coalesce = !hasSelection() && (len > 0u) && (len <= 4u) && !memchr(utf8_ch, '\n', len);
    return pasteInternal(utf8_ch, len, can_coalesce);
  }

  bool TextArea::rmChar()
  {
    if (hasSelection())
      return deleteSelected();

    if (_cursor_pos == 0u)
      return false;

    const size_t prev_pos = prevUtf8Pos(_cursor_pos);
    const size_t del_bytes = _cursor_pos - prev_pos;
    const bool removed_nl = (byteAt(prev_pos) == '\n');

    recordUndoStep(prev_pos, del_bytes, 0u, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    moveGap(_cursor_pos);
    _gap_start -= del_bytes;
    _cursor_pos = _gap_start;

    if (_char_len > 0u)
      --_char_len;

    if (removed_nl && _total_lines > 1u)
      --_total_lines;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(prev_pos < _view_top_byte);
    _is_changed = true;
    return true;
  }

  bool TextArea::delChar()
  {
    if (hasSelection())
      return deleteSelected();

    if (_cursor_pos >= getByteLen())
      return false;

    const uint8_t del_bytes = utf8LenAt(_cursor_pos);
    const bool removed_nl = (byteAt(_cursor_pos) == '\n');

    recordUndoStep(_cursor_pos, del_bytes, 0u, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    moveGap(_cursor_pos);
    _gap_end += del_bytes;

    if (_char_len > 0u)
      --_char_len;

    if (removed_nl && _total_lines > 1u)
      --_total_lines;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(_cursor_pos < _view_top_byte);
    _is_changed = true;
    return true;
  }

  void TextArea::clearText()
  {
    const size_t len = getByteLen();
    if (len > 0u)
      recordUndoStep(0u, len, 0u, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    _gap_start = 0u;
    _gap_end = _buf_cap;
    _cursor_pos = 0u;
    _sel_anchor = 0u;
    _view_top_byte = 0u;
    _char_len = 0u;
    _total_lines = 1u;
    _view_top_line_num = 1u;
    _preferred_x = -1;
    _is_selecting = false;
    _cursor_visible = true;
    _last_blink_ts = millis();
    _is_changed = true;
  }

  bool TextArea::undo()
  {
    if (!_undo_buf || _undo_count == 0u)
      return false;

    const uint8_t idx = static_cast<uint8_t>((_undo_head + _undo_count - 1u) % _undo_cap);
    UndoStep& step = _undo_buf[idx];

    const size_t grow_needed = (step.removed_len > step.inserted_len) ? (step.removed_len - step.inserted_len) : 0u;
    if (grow_needed > 0u && !ensureGap(grow_needed))
      return false;

    moveGap(step.pos);
    _gap_end += step.inserted_len;

    if (step.removed_len > 0u && step.removed_buf)
    {
      memcpy(_buf + _gap_start, step.removed_buf, step.removed_len);
      _gap_start += step.removed_len;
    }

    const size_t restored_cursor = step.prev_cursor;
    const size_t restored_anchor = step.prev_sel_anchor;
    const bool restored_selecting = step.prev_is_selecting;
    const uint32_t restored_char_len = step.prev_char_len;
    const uint32_t restored_total_lines = step.prev_total_lines;

    freeUndoStep(step);
    --_undo_count;

    _cursor_pos = restored_cursor;
    _sel_anchor = restored_anchor;
    _is_selecting = restored_selecting;
    _char_len = restored_char_len;
    _total_lines = restored_total_lines;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(true);
    _is_changed = true;
    return true;
  }

  bool TextArea::canUndo() const
  {
    return _undo_count > 0u;
  }

  void TextArea::clearUndoHistory()
  {
    while (_undo_count > 0u)
      evictOldestUndo();

    _undo_head = 0u;
    _undo_used_bytes = 0u;
  }

  void TextArea::setMaxUndoSteps(const uint8_t steps)
  {
    clearUndoHistory();
    if (_undo_buf)
    {
      free(_undo_buf);
      _undo_buf = nullptr;
    }

    _undo_cap = steps;
  }

  uint8_t TextArea::getMaxUndoSteps() const
  {
    return _undo_cap;
  }

  void TextArea::setUndoMemLimit(const size_t max_bytes)
  {
    _undo_mem_limit = max_bytes;
    while (_undo_count > 0u && _undo_used_bytes > _undo_mem_limit)
      evictOldestUndo();
  }

  size_t TextArea::getUndoMemLimit() const
  {
    return _undo_mem_limit;
  }

  const char* TextArea::getText() const
  {
    const size_t len = getByteLen();
    if (!_buf || len == 0u)
      return "";

    moveGap(len);
    _buf[len] = '\0';
    return _buf;
  }

  size_t TextArea::getByteLen() const
  {
    return _buf_cap - (_gap_end - _gap_start);
  }

  uint32_t TextArea::getCharLen() const
  {
    return _char_len;
  }

  uint32_t TextArea::getTotalLines() const
  {
    return _total_lines;
  }

  bool TextArea::isEmpty() const
  {
    return getByteLen() == 0u;
  }

  void TextArea::moveCursorLeft()
  {
    syncLayoutMetrics();
    if (_cursor_pos == 0u)
      return;

    _cursor_pos = prevUtf8Pos(_cursor_pos);
    _preferred_x = -1;
    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::moveCursorRight()
  {
    syncLayoutMetrics();
    const size_t len = getByteLen();
    if (_cursor_pos >= len)
      return;

    _cursor_pos += utf8LenAt(_cursor_pos);
    _preferred_x = -1;
    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::moveCursorUp()
  {
    syncLayoutMetrics();
    const ContentRect rect = getContentRect();
    if (rect.w == 0u || _cursor_pos == 0u)
      return;

    VisualLine cur_line{};
    findVisualLineAtPos(_cursor_pos, rect.w, cur_line);

    if (cur_line.start == 0u)
    {
      _cursor_pos = 0u;
      _preferred_x = -1;
      finishCursorMove(ensureCursorVisible());
      return;
    }

    if (_preferred_x < 0)
      _preferred_x = static_cast<int16_t>(measureRangeWidth(cur_line.start, _cursor_pos));

    VisualLine prev_line{};
    if (findPrevVisualLine(cur_line.start, rect.w, prev_line))
      _cursor_pos = closestPosAtX(prev_line, static_cast<uint16_t>(_preferred_x));
    else
      _cursor_pos = 0u;

    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::moveCursorDown()
  {
    syncLayoutMetrics();
    const ContentRect rect = getContentRect();
    const size_t len = getByteLen();
    if (rect.w == 0u || _cursor_pos >= len)
      return;

    VisualLine cur_line{};
    findVisualLineAtPos(_cursor_pos, rect.w, cur_line);

    const bool has_next = (cur_line.end < len) || cur_line.ends_with_nl;
    if (!has_next)
    {
      _cursor_pos = len;
      _preferred_x = -1;
      finishCursorMove(ensureCursorVisible());
      return;
    }

    if (_preferred_x < 0)
      _preferred_x = static_cast<int16_t>(measureRangeWidth(cur_line.start, _cursor_pos));

    VisualLine next_line{};
    calcVisualLine(cur_line.end, rect.w, next_line);
    _cursor_pos = closestPosAtX(next_line, static_cast<uint16_t>(_preferred_x));

    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::setCursorByPos(const uint16_t x, const uint16_t y)
  {
    syncLayoutMetrics();
    const ContentRect rect = getContentRect();
    const size_t len = getByteLen();

    if (len == 0u || rect.w == 0u)
    {
      _cursor_pos = 0u;
      _preferred_x = -1;
      finishCursorMove(false);
      return;
    }

    const uint16_t local_x = (x > rect.x) ? (x - rect.x) : 0u;
    const uint16_t local_y = (y > rect.y) ? (y - rect.y) : 0u;
    const uint16_t target_row = (_line_step > 0u) ? (local_y / _line_step) : 0u;

    size_t pos = _view_top_byte;
    VisualLine line{};
    bool found_row{false};

    for (uint16_t row{0u}; row < _visible_lines; ++row)
    {
      calcVisualLine(pos, rect.w, line);

      if (row == target_row)
      {
        found_row = true;
        break;
      }

      if (line.end >= len && !line.ends_with_nl)
        break;

      pos = line.end;
    }

    if (found_row)
      _cursor_pos = closestPosAtX(line, local_x);
    else
      _cursor_pos = len;

    _preferred_x = -1;
    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::setCursorToStart()
  {
    syncLayoutMetrics();
    if (_cursor_pos == 0u)
      return;

    _cursor_pos = 0u;
    _preferred_x = -1;
    finishCursorMove(ensureCursorVisible());
  }

  void TextArea::setCursorToEnd()
  {
    syncLayoutMetrics();
    const size_t len = getByteLen();
    if (_cursor_pos == len)
      return;

    _cursor_pos = len;
    _preferred_x = -1;
    finishCursorMove(ensureCursorVisible());
  }

  size_t TextArea::getCursorBytePos() const
  {
    return _cursor_pos;
  }

  void TextArea::setSelectionMode(const bool state)
  {
    if (state)
    {
      if (!_is_selecting)
      {
        _is_selecting = true;
        _sel_anchor = _cursor_pos;
      }
    }
    else
    {
      clearSelection();
    }
  }

  bool TextArea::isSelectionMode() const
  {
    return _is_selecting;
  }

  bool TextArea::hasSelection() const
  {
    return _is_selecting && _sel_anchor != _cursor_pos;
  }

  void TextArea::clearSelection()
  {
    const bool had_selection = hasSelection();
    _is_selecting = false;
    _sel_anchor = _cursor_pos;

    if (had_selection)
      _is_changed = true;
  }

  bool TextArea::copySelected()
  {
    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);
    const size_t sel_len = sel_end - sel_start;

    if (sel_len == 0u)
      return false;

    char* new_clip = nullptr;
    if (psramInit())
      new_clip = static_cast<char*>(_clip_buf ? ps_realloc(_clip_buf, sel_len) : ps_malloc(sel_len));
    else
      new_clip = static_cast<char*>(_clip_buf ? realloc(_clip_buf, sel_len) : malloc(sel_len));

    if (!new_clip)
    {
      log_e("Помилка виділення пам'яті для буфера обміну TextArea (%u байт)", static_cast<uint32_t>(sel_len));
      return false;
    }

    _clip_buf = new_clip;
    _clip_len = sel_len;
    copyRange(sel_start, sel_len, _clip_buf);
    return true;
  }

  bool TextArea::cutSelected()
  {
    if (!copySelected())
      return false;

    return deleteSelected();
  }

  String TextArea::getSelectedText() const
  {
    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);
    const size_t sel_len = sel_end - sel_start;

    if (sel_len == 0u)
      return emptyString;

    String result;
    if (!result.reserve(sel_len))
    {
      log_e("Помилка виділення пам'яті для отримання виділеного тексту (%u байт)", static_cast<uint32_t>(sel_len));
      return emptyString;
    }

    char chunk[DRAW_CHUNK_SIZE + 1u];
    size_t pos = sel_start;

    while (pos < sel_end)
    {
      const size_t remaining = sel_end - pos;
      const size_t chunk_len = (remaining < DRAW_CHUNK_SIZE) ? remaining : DRAW_CHUNK_SIZE;

      for (size_t i{0u}; i < chunk_len; ++i)
        chunk[i] = byteAt(pos + i);

      chunk[chunk_len] = '\0';
      result += chunk;
      pos += chunk_len;
    }

    return result;
  }

  bool TextArea::hasClipboard() const
  {
    return _clip_buf != nullptr && _clip_len > 0u;
  }

  void TextArea::clearClipboard()
  {
    if (_clip_buf)
    {
      free(_clip_buf);
      _clip_buf = nullptr;
    }

    _clip_len = 0u;
  }

  bool TextArea::deleteSelected()
  {
    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);
    const size_t sel_len = sel_end - sel_start;

    if (sel_len == 0u)
      return false;

    recordUndoStep(sel_start, sel_len, 0u, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    uint32_t removed_chars{0u};
    uint32_t removed_nl{0u};
    for (size_t i{sel_start}; i < sel_end; ++i)
    {
      const char c = byteAt(i);
      if (c == '\n')
        ++removed_nl;

      if ((static_cast<uint8_t>(c) & 0xC0u) != 0x80u)
        ++removed_chars;
    }

    moveGap(sel_start);
    _gap_end += sel_len;
    _cursor_pos = sel_start;
    _char_len = (_char_len > removed_chars) ? (_char_len - removed_chars) : 0u;
    _total_lines = (_total_lines > removed_nl) ? (_total_lines - removed_nl) : 1u;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(sel_start < _view_top_byte);
    _is_changed = true;
    return true;
  }

  bool TextArea::duplicateSelected()
  {
    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);
    const size_t sel_len = sel_end - sel_start;

    if (sel_len == 0u)
      return false;

    if (!ensureGap(sel_len))
      return false;

    recordUndoStep(sel_end, 0u, sel_len, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    moveGap(sel_end);

    uint32_t added_chars{0u};
    uint32_t added_nl{0u};
    for (size_t i{sel_start}; i < sel_end; ++i)
    {
      const char c = _buf[i];
      if (c == '\n')
        ++added_nl;

      if ((static_cast<uint8_t>(c) & 0xC0u) != 0x80u)
        ++added_chars;
    }

    memcpy(_buf + _gap_start, _buf + sel_start, sel_len);
    _gap_start += sel_len;
    _cursor_pos = _gap_start;
    _char_len += added_chars;
    _total_lines += added_nl;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(sel_end < _view_top_byte);
    _is_changed = true;
    return true;
  }

  bool TextArea::duplicateLine()
  {
    const size_t len = getByteLen();

    size_t line_start = _cursor_pos;
    while (line_start > 0u && byteAt(line_start - 1u) != '\n')
      --line_start;

    size_t line_end = _cursor_pos;
    while (line_end < len && byteAt(line_end) != '\n')
      ++line_end;

    const size_t line_len = line_end - line_start;
    const size_t cursor_offset = _cursor_pos - line_start;
    const size_t insert_len = line_len + 1u;

    if (!ensureGap(insert_len))
      return false;

    recordUndoStep(line_end, 0u, insert_len, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    moveGap(line_end);

    uint32_t added_chars{1u};
    for (size_t i{line_start}; i < line_end; ++i)
    {
      if ((static_cast<uint8_t>(_buf[i]) & 0xC0u) != 0x80u)
        ++added_chars;
    }

    _buf[_gap_start] = '\n';
    if (line_len > 0u)
      memcpy(_buf + _gap_start + 1u, _buf + line_start, line_len);

    _gap_start += insert_len;
    _cursor_pos = line_end + 1u + cursor_offset;
    _char_len += added_chars;
    ++_total_lines;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(line_end < _view_top_byte);
    _is_changed = true;
    return true;
  }

  bool TextArea::cutLine()
  {
    const size_t len = getByteLen();
    if (len == 0u)
      return false;

    size_t line_start = _cursor_pos;
    while (line_start > 0u && byteAt(line_start - 1u) != '\n')
      --line_start;

    size_t line_end = _cursor_pos;
    while (line_end < len && byteAt(line_end) != '\n')
      ++line_end;

    size_t cut_start = line_start;
    size_t cut_end = line_end;
    size_t clip_start = line_start;
    size_t clip_end = line_end;

    if (line_end < len)
    {
      cut_end = line_end + 1u;
      clip_end = cut_end;
    }
    else if (line_start > 0u)
    {
      cut_start = line_start - 1u;
      if (line_end == line_start)
        clip_start = cut_start;
    }

    const size_t cut_len = cut_end - cut_start;
    const size_t clip_len = clip_end - clip_start;
    if (cut_len == 0u || clip_len == 0u)
      return false;

    char* new_clip = nullptr;
    if (psramInit())
      new_clip = static_cast<char*>(_clip_buf ? ps_realloc(_clip_buf, clip_len) : ps_malloc(clip_len));
    else
      new_clip = static_cast<char*>(_clip_buf ? realloc(_clip_buf, clip_len) : malloc(clip_len));

    if (!new_clip)
    {
      log_e("Помилка виділення пам'яті для буфера обміну TextArea (%u байт)", static_cast<uint32_t>(clip_len));
      return false;
    }

    _clip_buf = new_clip;
    _clip_len = clip_len;
    copyRange(clip_start, clip_len, _clip_buf);

    recordUndoStep(cut_start, cut_len, 0u, _cursor_pos, _sel_anchor, _is_selecting, _char_len, _total_lines, false);

    uint32_t removed_chars{0u};
    uint32_t removed_nl{0u};
    for (size_t i{cut_start}; i < cut_end; ++i)
    {
      const char c = byteAt(i);
      if (c == '\n')
        ++removed_nl;

      if ((static_cast<uint8_t>(c) & 0xC0u) != 0x80u)
        ++removed_chars;
    }

    moveGap(cut_start);
    _gap_end += cut_len;
    _cursor_pos = cut_start;
    _char_len = (_char_len > removed_chars) ? (_char_len - removed_chars) : 0u;
    _total_lines = (_total_lines > removed_nl) ? (_total_lines - removed_nl) : 1u;

    _is_selecting = false;
    _sel_anchor = _cursor_pos;
    _preferred_x = -1;
    _cursor_visible = true;
    _last_blink_ts = millis();

    syncViewportAfterEdit(cut_start < _view_top_byte);
    _is_changed = true;
    return true;
  }

  void TextArea::setShowLineNumbers(const bool state)
  {
    if (_show_line_numbers == state)
      return;

    _show_line_numbers = state;
    _cached_width = 0u;
    _is_changed = true;
  }

  bool TextArea::hasLineNumbers() const
  {
    return _show_line_numbers;
  }

  void TextArea::setLineNumBackColor(const uint16_t color)
  {
    _line_num_back_color = color;
    if (_show_line_numbers)
      _is_changed = true;
  }

  uint16_t TextArea::getLineNumBackColor() const
  {
    return _line_num_back_color;
  }

  void TextArea::setLineNumTextColor(const uint16_t color)
  {
    _line_num_text_color = color;
    if (_show_line_numbers)
      _is_changed = true;
  }

  uint16_t TextArea::getLineNumTextColor() const
  {
    return _line_num_text_color;
  }

  void TextArea::setLineNumBorderColor(const uint16_t color)
  {
    _line_num_border_color = color;
    if (_show_line_numbers)
      _is_changed = true;
  }

  uint16_t TextArea::getLineNumBorderColor() const
  {
    return _line_num_border_color;
  }

  void TextArea::setHint(const char* hint)
  {
    _hint = hint ? hint : "";
    if (isEmpty())
      _is_changed = true;
  }

  void TextArea::setHint(const String& hint)
  {
    _hint = hint;
    if (isEmpty())
      _is_changed = true;
  }

  const String& TextArea::getHint() const
  {
    return _hint;
  }

  void TextArea::setHintColor(const uint16_t color)
  {
    _hint_color = color;
    if (isEmpty() && !_hint.isEmpty())
      _is_changed = true;
  }

  uint16_t TextArea::getHintColor() const
  {
    return _hint_color;
  }

  void TextArea::setFont(const uint8_t* font)
  {
    _font = font ? font : font_unifont;
    updateFontMetrics();
  }

  const uint8_t* TextArea::getFont() const
  {
    return _font;
  }

  void TextArea::setTextSize(uint8_t size)
  {
    if (size == 0u)
      size = 1u;

    _text_size = size;
    updateFontMetrics();
  }

  uint8_t TextArea::getTextSize() const
  {
    return _text_size;
  }

  void TextArea::setTextColor(const uint16_t color)
  {
    _text_color = color;
    _is_changed = true;
  }

  uint16_t TextArea::getTextColor() const
  {
    return _text_color;
  }

  void TextArea::setCursorColor(const uint16_t color)
  {
    _cursor_color = color;
    _is_cursor_dirty = true;
  }

  uint16_t TextArea::getCursorColor() const
  {
    return _cursor_color;
  }

  void TextArea::setCursorBlinkDelay(const uint16_t delay_ms)
  {
    _cursor_blink_delay = (delay_ms < MIN_BLINK_DELAY) ? MIN_BLINK_DELAY : delay_ms;
  }

  uint16_t TextArea::getCursorBlinkDelay() const
  {
    return _cursor_blink_delay;
  }

  void TextArea::setSelectionBackColor(const uint16_t color)
  {
    _sel_back_color = color;
    if (hasSelection())
      _is_changed = true;
  }

  uint16_t TextArea::getSelectionBackColor() const
  {
    return _sel_back_color;
  }

  void TextArea::setSelectionTextColor(const uint16_t color)
  {
    _sel_text_color = color;
    if (hasSelection())
      _is_changed = true;
  }

  uint16_t TextArea::getSelectionTextColor() const
  {
    return _sel_text_color;
  }

  void TextArea::setHPadding(const uint8_t padding)
  {
    _h_padding = padding;
    _cached_width = 0u;
    _is_changed = true;
  }

  void TextArea::setVPadding(const uint8_t padding)
  {
    _v_padding = padding;
    _cached_height = 0u;
    _is_changed = true;
  }

  uint16_t TextArea::getCharHgt() const
  {
    return _char_hgt;
  }

  uint16_t TextArea::getLineHeight() const
  {
    return _line_step;
  }

  uint16_t TextArea::getVisibleLines() const
  {
    return _visible_lines;
  }

  void TextArea::drawLineNumber(const uint32_t line_num, const uint16_t gutter_x, const uint16_t gutter_w, const uint16_t rect_y, const uint16_t rect_h, const uint16_t line_top_y) const
  {
    if (gutter_w <= 1u)
      return;

    char num_buf[12];
    const int written = snprintf(num_buf, sizeof(num_buf), "%lu", static_cast<unsigned long>(line_num));
    if (written <= 0)
      return;

    const uint16_t inner_w = gutter_w - 1u;
    const uint16_t text_w = static_cast<uint16_t>(written) * _digit_w;
    const uint16_t right_edge = (inner_w > GUTTER_HPAD) ? (inner_w - GUTTER_HPAD) : inner_w;
    const uint16_t offset_x = (right_edge > text_w) ? (right_edge - text_w) : 0u;
    const int16_t baseline_y = static_cast<int16_t>(line_top_y) - _y_char_offset;

    _display.setTextBound(gutter_x, rect_y, inner_w, rect_h);
    _display.setTextColor(_line_num_text_color);
    _display.setCursor(gutter_x + offset_x, baseline_y);
    _display.print(num_buf);
  }

  void TextArea::drawTextSlice(const size_t from, const size_t to) const
  {
    if (from >= to)
      return;

    char chunk[DRAW_CHUNK_SIZE + 1u];
    size_t pos = from;

    while (pos < to)
    {
      size_t chunk_len{0u};

      while (pos < to)
      {
        const char c = byteAt(pos);
        if (c == '\t')
        {
          if (chunk_len + TAB_SPACES > DRAW_CHUNK_SIZE)
            break;

          for (uint8_t s{0u}; s < TAB_SPACES; ++s)
            chunk[chunk_len++] = ' ';

          ++pos;
          continue;
        }

        const uint8_t clen = utf8LenAt(pos);
        if (clen == 0u || pos + clen > to)
          break;

        if (chunk_len + clen > DRAW_CHUNK_SIZE)
          break;

        for (uint8_t i{0u}; i < clen; ++i)
          chunk[chunk_len++] = byteAt(pos + i);

        pos += clen;
      }

      if (chunk_len == 0u)
        break;

      chunk[chunk_len] = '\0';
      _display.print(chunk);
    }
  }

  void TextArea::drawVisualLine(const VisualLine& line, const ContentRect& rect, const uint16_t line_top_y, const size_t sel_start, const size_t sel_end) const
  {
    const size_t vis_end = line.ends_with_nl ? (line.end - 1u) : line.end;
    const int16_t baseline_y = static_cast<int16_t>(line_top_y) - _y_char_offset;
    const bool has_overlap = (sel_end > sel_start) && (sel_start < line.end) && (sel_end > line.start);

    if (!has_overlap)
    {
      if (vis_end > line.start)
      {
        _display.setTextColor(_text_color);
        _display.setCursor(rect.x, baseline_y);
        drawTextSlice(line.start, vis_end);
      }
      return;
    }

    const size_t hl_start = (sel_start > line.start) ? sel_start : line.start;
    const size_t hl_end_vis = (sel_end < vis_end) ? sel_end : vis_end;
    const bool nl_selected = line.ends_with_nl && (sel_end >= line.end);

    const uint16_t pre_w = measureRangeWidth(line.start, hl_start);
    const uint16_t sel_text_w = (hl_end_vis > hl_start) ? measureRangeWidth(hl_start, hl_end_vis) : 0u;
    uint16_t hl_w = sel_text_w;

    if (nl_selected)
    {
      const uint8_t space_w = getCharWidth(' ', " ", 1u);
      hl_w += (space_w > MIN_NL_HIGHLIGHT_W) ? space_w : MIN_NL_HIGHLIGHT_W;
    }

    if (hl_w > 0u && rect.w > pre_w)
    {
      const uint16_t max_w = rect.w - pre_w;
      const uint16_t draw_w = (hl_w < max_w) ? hl_w : max_w;
      const uint16_t content_bottom = rect.y + rect.h;
      const uint16_t avail_h = (content_bottom > line_top_y) ? (content_bottom - line_top_y) : 0u;
      const uint16_t draw_h = (_line_step < avail_h) ? _line_step : avail_h;

      if (draw_w > 0u && draw_h > 0u)
        _display.fillRect(rect.x + pre_w, line_top_y, draw_w, draw_h, _sel_back_color);
    }

    if (hl_start > line.start)
    {
      _display.setTextColor(_text_color);
      _display.setCursor(rect.x, baseline_y);
      drawTextSlice(line.start, hl_start);
    }

    if (hl_end_vis > hl_start)
    {
      _display.setTextColor(_sel_text_color);
      _display.setCursor(rect.x + pre_w, baseline_y);
      drawTextSlice(hl_start, hl_end_vis);
    }

    if (vis_end > hl_end_vis)
    {
      _display.setTextColor(_text_color);
      _display.setCursor(rect.x + pre_w + sel_text_w, baseline_y);
      drawTextSlice(hl_end_vis, vis_end);
    }
  }

  void TextArea::drawCursorAt(const int16_t x, const int16_t y, const size_t pos)
  {
    const ContentRect rect = getContentRect();
    if (rect.w == 0u || rect.h == 0u)
      return;

    const int16_t rect_right = static_cast<int16_t>(rect.x + rect.w);
    const int16_t rect_bottom = static_cast<int16_t>(rect.y + rect.h);

    int16_t draw_x = x;
    if (draw_x >= rect_right)
      draw_x = rect_right - static_cast<int16_t>(CURSOR_WIDTH);

    if (draw_x < static_cast<int16_t>(rect.x) || y < static_cast<int16_t>(rect.y) || y >= rect_bottom)
      return;

    const int16_t avail_w = rect_right - draw_x;
    const int16_t avail_h = rect_bottom - y;
    const uint16_t draw_w = (avail_w < static_cast<int16_t>(CURSOR_WIDTH)) ? static_cast<uint16_t>(avail_w) : CURSOR_WIDTH;
    const uint16_t draw_h = (avail_h < static_cast<int16_t>(_char_hgt)) ? static_cast<uint16_t>(avail_h) : _char_hgt;

    if (draw_w == 0u || draw_h == 0u)
      return;

    _display.fillRect(draw_x, y, draw_w, draw_h, _cursor_color);
    _drawn_cursor_x = draw_x;
    _drawn_cursor_y = y;
    _drawn_cursor_pos = pos;
    _is_cursor_drawn = true;
  }

  void TextArea::eraseDrawnCursor()
  {
    if (!_is_cursor_drawn)
      return;

    _is_cursor_drawn = false;

    const ContentRect rect = getContentRect();
    if (rect.w == 0u || rect.h == 0u)
      return;

    const int16_t rect_right = static_cast<int16_t>(rect.x + rect.w);
    const int16_t rect_bottom = static_cast<int16_t>(rect.y + rect.h);
    if (_drawn_cursor_x < static_cast<int16_t>(rect.x) || _drawn_cursor_x >= rect_right ||
        _drawn_cursor_y < static_cast<int16_t>(rect.y) || _drawn_cursor_y >= rect_bottom)
    {
      return;
    }

    const int16_t avail_w = rect_right - _drawn_cursor_x;
    const int16_t avail_h = rect_bottom - _drawn_cursor_y;
    const uint16_t draw_w = (avail_w < static_cast<int16_t>(CURSOR_WIDTH)) ? static_cast<uint16_t>(avail_w) : CURSOR_WIDTH;
    const uint16_t draw_h = (avail_h < static_cast<int16_t>(_char_hgt)) ? static_cast<uint16_t>(avail_h) : _char_hgt;

    // --- замінити все від цього місця (включно зі старими sel_start/sel_end та fillRect) до кінця методу ---

    const int16_t baseline_y = _drawn_cursor_y - _y_char_offset;

    if (isEmpty())
    {
      _display.fillRect(_drawn_cursor_x, _drawn_cursor_y, draw_w, draw_h, _back_color);

      if (!_hint.isEmpty() && _drawn_cursor_x == static_cast<int16_t>(rect.x) && _drawn_cursor_y == static_cast<int16_t>(rect.y))
      {
        const uint8_t b0 = static_cast<uint8_t>(_hint[0]);
        uint8_t first_len{1u};
        if ((b0 & 0xE0u) == 0xC0u)
          first_len = 2u;
        else if ((b0 & 0xF0u) == 0xE0u)
          first_len = 3u;
        else if ((b0 & 0xF8u) == 0xF0u)
          first_len = 4u;

        if (first_len > _hint.length())
          first_len = static_cast<uint8_t>(_hint.length());

        char first_ch[5];
        memcpy(first_ch, _hint.c_str(), first_len);
        first_ch[first_len] = '\0';

        _display.setFont(_font);
        _display.setTextSize(_text_size);
        _display.setTextWrap(false);
        _display.setTextBound(rect.x, rect.y, rect.w, rect.h);
        _display.setTextColor(_hint_color);
        _display.setCursor(rect.x, baseline_y);
        _display.print(first_ch);
        _display.resetTextBound();
      }

      return;
    }

    VisualLine line{};
    findVisualLineAtPos(_drawn_cursor_pos, rect.w, line);
    const size_t vis_end = line.ends_with_nl ? (line.end - 1u) : line.end;
    const int16_t logical_x = static_cast<int16_t>(rect.x + measureRangeWidth(line.start, _drawn_cursor_pos));

    size_t sel_start{0u};
    size_t sel_end{0u};
    getSelectionRange(sel_start, sel_end);

    const bool pos_in_sel = (sel_end > sel_start) && (_drawn_cursor_pos >= sel_start) && (_drawn_cursor_pos < sel_end);
    const size_t prev_pos = (_drawn_cursor_pos > line.start) ? prevUtf8Pos(_drawn_cursor_pos) : _drawn_cursor_pos;
    const bool prev_in_sel = (_drawn_cursor_pos > line.start) && (sel_end > sel_start) && (prev_pos >= sel_start) && (prev_pos < sel_end);

    // Якщо курсор було притиснуто до правого краю, він перекриває попередній символ
    const bool bg_in_sel = (_drawn_cursor_x < logical_x && _drawn_cursor_pos > line.start) ? prev_in_sel : pos_in_sel;
    _display.fillRect(_drawn_cursor_x, _drawn_cursor_y, draw_w, draw_h, bg_in_sel ? _sel_back_color : _back_color);

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextWrap(false);
    _display.setTextBound(rect.x, rect.y, rect.w, rect.h);

    // Відновлюємо символ під курсором та сусідній символ зліва на випадок перекриття гліфів
    if (_drawn_cursor_pos < vis_end)
    {
      const uint8_t clen = utf8LenAt(_drawn_cursor_pos);
      if (clen > 0u)
      {
        _display.setTextColor(pos_in_sel ? _sel_text_color : _text_color);
        _display.setCursor(logical_x, baseline_y);
        drawTextSlice(_drawn_cursor_pos, _drawn_cursor_pos + clen);
      }
    }

    if (_drawn_cursor_pos > line.start)
    {
      uint8_t prev_len{0u};
      const uint8_t prev_w = getCharWidthAt(prev_pos, prev_len);
      if (prev_len > 0u && logical_x >= static_cast<int16_t>(rect.x + prev_w))
      {
        _display.setTextColor(prev_in_sel ? _sel_text_color : _text_color);
        _display.setCursor(logical_x - prev_w, baseline_y);
        drawTextSlice(prev_pos, _drawn_cursor_pos);
      }
    }

    _display.resetTextBound();
  }

  void TextArea::onDraw()
  {
    if (_visibility == INVISIBLE)
    {
      if (_is_changed)
      {
        _is_changed = false;
        _is_cursor_dirty = false;
        _is_cursor_drawn = false;
        hide();
      }
      return;
    }

    if (!_is_changed)
    {
      if (_is_cursor_dirty)
      {
        _is_cursor_dirty = false;
        eraseDrawnCursor();
        if (_cursor_visible)
          drawCursorAt(_cursor_x, _cursor_y, _cursor_pos);

        return;
      }

      const unsigned long now = millis();
      if ((now - _last_blink_ts) < _cursor_blink_delay)
        return;

      _last_blink_ts = now;
      _cursor_visible = !_cursor_visible;

      if (_cursor_visible)
        drawCursorAt(_cursor_x, _cursor_y, _cursor_pos);
      else
        eraseDrawnCursor();

      return;
    }

    _is_changed = false;
    _is_cursor_dirty = false;
    _is_cursor_drawn = false;

    syncLayoutMetrics();
    clear();

    const ContentRect rect = getContentRect();
    if (rect.w == 0u || rect.h == 0u)
      return;

    const uint16_t gutter_w = calcGutterWidth();
    const uint16_t border = _has_border ? 1u : 0u;
    const uint16_t gutter_x = getXPos() + border;
    const uint16_t inner_y = getYPos() + border;
    const uint16_t inner_h = (_height > border * 2u) ? (_height - border * 2u) : 0u;

    if (gutter_w > 1u && inner_h > 0u)
    {
      _display.fillRect(gutter_x, inner_y, gutter_w - 1u, inner_h, _line_num_back_color);
      _display.drawLine(gutter_x + gutter_w - 1u, inner_y, gutter_x + gutter_w - 1u, inner_y + inner_h - 1u, _line_num_border_color);
    }

    _display.setFont(_font);
    _display.setTextSize(_text_size);
    _display.setTextWrap(false);

    if (isEmpty())
    {
      if (gutter_w > 1u)
        drawLineNumber(1u, gutter_x, gutter_w, rect.y, rect.h, rect.y);

      if (!_hint.isEmpty())
      {
        _display.setTextBound(rect.x, rect.y, rect.w, rect.h);
        _display.setTextColor(_hint_color);
        _display.setTextWrap(true);
        _display.setCursor(rect.x, static_cast<int16_t>(rect.y) - _y_char_offset);
        _display.print(_hint.c_str());
        _display.setTextWrap(false);
      }

      _cursor_x = static_cast<int16_t>(rect.x);
      _cursor_y = static_cast<int16_t>(rect.y);
    }
    else
    {
      size_t sel_start{0u};
      size_t sel_end{0u};
      getSelectionRange(sel_start, sel_end);

      const size_t len = getByteLen();
      size_t pos = _view_top_byte;
      uint32_t cur_line_num = _view_top_line_num;
      bool cursor_located{false};

      if (gutter_w == 0u)
        _display.setTextBound(rect.x, rect.y, rect.w, rect.h);

      for (uint16_t row{0u}; row < _visible_lines; ++row)
      {
        VisualLine line{};
        calcVisualLine(pos, rect.w, line);

        const uint16_t line_top_y = rect.y + row * _line_step;

        if (gutter_w > 1u)
        {
          const bool is_logical_start = (line.start == 0u) || (byteAt(line.start - 1u) == '\n');
          if (is_logical_start)
            drawLineNumber(cur_line_num, gutter_x, gutter_w, rect.y, rect.h, line_top_y);

          _display.setTextBound(rect.x, rect.y, rect.w, rect.h);
        }

        drawVisualLine(line, rect, line_top_y, sel_start, sel_end);

        if (!cursor_located && lineOwnsCursor(line, _cursor_pos))
        {
          const uint16_t offset_x = measureRangeWidth(line.start, _cursor_pos);
          _cursor_x = static_cast<int16_t>(rect.x + offset_x);
          _cursor_y = static_cast<int16_t>(line_top_y);
          cursor_located = true;
        }

        if (line.ends_with_nl)
          ++cur_line_num;

        if (line.end >= len && !line.ends_with_nl)
          break;

        pos = line.end;
      }

      if (!cursor_located)
      {
        _cursor_x = static_cast<int16_t>(rect.x);
        _cursor_y = static_cast<int16_t>(rect.y);
      }
    }

    _display.resetTextBound();

    if (_cursor_visible)
      drawCursorAt(_cursor_x, _cursor_y, _cursor_pos);
  }
}  // namespace pixeler
