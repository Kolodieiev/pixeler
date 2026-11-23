#pragma GCC optimize("O3")

#include "EditorContext.h"

#include <cstring>

#include "../WidgetCreator.h"
#include "context/menu/MenuContext.h"
#include "widget/layout/EmptyLayout.h"

static constexpr uint8_t MOD_CTRL_MASK{0x01u | 0x10u};
static constexpr uint8_t MOD_SHIFT_MASK{0x02u | 0x20u};
static constexpr uint8_t MOD_ALT_MASK{0x04u | 0x40u};

static constexpr uint8_t ASCII_PRINTABLE_MIN{0x20u};
static constexpr uint8_t ASCII_DEL{0x7Fu};
static constexpr uint16_t CURSOR_BLINK_MS{450u};

static const char STR_EDITOR_HINT[] = "Введіть текст...";

EditorContext::EditorContext()
{
  setCpuFrequency(FREQ_MIN);

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _editor = new TextArea(ID_TEXTAREA);
  layout->addWidget(_editor);
  _editor->setPos(0, 0);
  _editor->setWidth(UI_WIDTH);
  _editor->setHeight(UI_HEIGHT);
  _editor->setBackColor(COLOR_MAIN_BACK);
  _editor->setTextColor(COLOR_WHITE);
  _editor->setCursorColor(COLOR_WHITE);
  _editor->setCursorBlinkDelay(CURSOR_BLINK_MS);
  _editor->setHint(STR_EDITOR_HINT);
  _editor->setShowLineNumbers(true);

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  _input.onKeyPressed(keyPressedHandler, this);
  _input.setKeyboardLayout(ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US);
#endif
}

EditorContext::~EditorContext()
{
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  _input.onKeyPressed(nullptr, nullptr);
#endif
}

bool EditorContext::loop()
{
  return true;
}

void EditorContext::update()
{
  if (_input.isHolded(BtnID::BTN_UP))
    _editor->moveCursorUp();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _editor->moveCursorDown();
  else if (_input.isReleased(BtnID::BTN_BACK))
    exitToMenu();

#ifdef TOUCHSCREEN_SUPPORT
  if (_input.isReleased())
    _editor->setCursorByPos(_input.getTouchX(), _input.getTouchY());
#endif
}

void EditorContext::handleCursorMove(const CursorAction action, const bool select)
{
  if (select)
    _editor->setSelectionMode(true);
  else
    _editor->clearSelection();

  switch (action)
  {
    case CUR_LEFT:
      _editor->moveCursorLeft();
      break;
    case CUR_RIGHT:
      _editor->moveCursorRight();
      break;
    case CUR_UP:
      _editor->moveCursorUp();
      break;
    case CUR_DOWN:
      _editor->moveCursorDown();
      break;
    case CUR_START:
      _editor->setCursorToStart();
      break;
    case CUR_END:
      _editor->setCursorToEnd();
      break;
  }
}

void EditorContext::toggleKeyboardLayout()
{
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  _is_ua_layout = !_is_ua_layout;
  _input.setKeyboardLayout(_is_ua_layout ? ESP_USB_HOST_KEYBOARD_LAYOUT_UK_UA : ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US);
#endif
}

void EditorContext::selectAll()
{
  _editor->clearSelection();
  _editor->setCursorToStart();
  _editor->setSelectionMode(true);
  _editor->setCursorToEnd();
}

void EditorContext::exitToMenu()
{
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  _input.onKeyPressed(nullptr, nullptr);
  _input.onKeyReleased(nullptr, nullptr);
#endif
  openContext(new MenuContext());
}

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
void EditorContext::keyPressedHandler(const EspUsbHostKeyboardEvent& event, void* arg)
{
  EditorContext* self = static_cast<EditorContext*>(arg);

  const bool ctrl = (event.modifiers & MOD_CTRL_MASK) != 0u;
  const bool shift = (event.modifiers & MOD_SHIFT_MASK) != 0u;
  const bool alt = (event.modifiers & MOD_ALT_MASK) != 0u;

  if (ctrl)
  {
    switch (event.keycode)
    {
      case HID_KEY_SPACE:
        self->post([self]()
                   { self->toggleKeyboardLayout(); });
        return;

      case HID_KEY_C:
        self->post([self]()
                   { self->_editor->copySelected(); });
        return;

      case HID_KEY_V:
        self->post([self]()
                   { self->_editor->paste(); });
        return;

      case HID_KEY_D:
        self->post([self]()
                   {
          if (self->_editor->hasSelection())
            self->_editor->duplicateSelected();
          else
            self->_editor->duplicateLine(); });
        return;

      case HID_KEY_Z:
        self->post([self]()
                   { self->_editor->undo(); });
        return;

      case HID_KEY_A:
        self->post([self]()
                   { self->selectAll(); });
        return;

      case HID_KEY_L:
        self->post([self]()
                   { self->_editor->setShowLineNumbers(!self->_editor->hasLineNumbers()); });
        return;

      case HID_KEY_X:
        self->post([self]()
                   {
          if (self->_editor->hasSelection())
            self->_editor->cutSelected();
          else
            self->_editor->cutLine(); });
        return;

      default:
        return;
    }
  }

  switch (event.keycode)
  {
    case Input::KEY_LEFT_ARROW:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_LEFT, shift); });
      return;

    case Input::KEY_RIGHT_ARROW:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_RIGHT, shift); });
      return;

    case Input::KEY_UP_ARROW:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_UP, shift); });
      return;

    case Input::KEY_DOWN_ARROW:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_DOWN, shift); });
      return;

    case HID_KEY_HOME:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_START, shift); });
      return;

    case HID_KEY_END:
      self->post([self, shift]()
                 { self->handleCursorMove(CUR_END, shift); });
      return;

    case Input::KEY_BACKSPACE:
      self->post([self]()
                 { self->_editor->rmChar(); });
      return;

    case HID_KEY_DELETE:
      self->post([self]()
                 { self->_editor->delChar(); });
      return;

    case Input::KEY_ENTER:
    case Input::KEY_ENTER_KP:
      self->post([self]()
                 { self->_editor->addChar('\n'); });
      return;

    case Input::KEY_TAB:
      self->post([self]()
                 { self->_editor->addChar('\t'); });
      return;

    case Input::KEY_ESCAPE:
      self->post([self]()
                 {
        if (self->_editor->hasSelection())
          self->_editor->clearSelection();
        else
          self->exitToMenu(); });
      return;

    default:
      break;
  }

  if (alt)
    return;

  if (event.utf8[0] != '\0')
  {
    const uint8_t first = static_cast<uint8_t>(event.utf8[0]);
    if (first >= ASCII_PRINTABLE_MIN && first != ASCII_DEL)
    {
      Utf8Char ch{};
      memcpy(ch.bytes, event.utf8, sizeof(event.utf8));
      ch.bytes[sizeof(event.utf8)] = '\0';
      self->post([self, ch]()
                 { self->_editor->addChar(ch.bytes); });
    }
    return;
  }

  if (event.ascii >= ASCII_PRINTABLE_MIN && event.ascii < ASCII_DEL)
  {
    const char ascii_ch = static_cast<char>(event.ascii);
    self->post([self, ascii_ch]()
               { self->_editor->addChar(ascii_ch); });
  }
}

#endif
