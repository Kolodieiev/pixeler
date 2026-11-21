#include "EspUsbHostHid.h"

#ifndef CONFIG_IDF_TARGET_ESP32

#include "keymap/keymap_en_us.h"
#include "keymap/keymap_uk_ua.h"

static constexpr uint8_t MOD_LEFT_SHIFT = 0x02;
static constexpr uint8_t MOD_RIGHT_SHIFT = 0x20;
static constexpr uint8_t MOD_RIGHT_ALT = 0x40;

static bool isCapsLockAffected(uint8_t keycode, EspUsbHostKeyboardLayout layout)
{
  if (keycode >= 0x04 && keycode <= 0x1D)
    return true;
  if (layout == ESP_USB_HOST_KEYBOARD_LAYOUT_UK_UA)
  {
    return keycode == 0x2F || keycode == 0x30 || keycode == 0x33 ||
        keycode == 0x34 || keycode == 0x36 || keycode == 0x37 ||
        keycode == 0x64;
  }
  return false;
}

uint8_t espUsbHostKeypadKeycodeToAscii(uint8_t keycode, bool numLock)
{
  switch (keycode)
  {
    case 0x54:
      return '/';
    case 0x55:
      return '*';
    case 0x56:
      return '-';
    case 0x57:
      return '+';
    case 0x58:
      return '\r';
    case 0x67:
      return '=';
    default:
      break;
  }
  if (!numLock)
    return 0;
  switch (keycode)
  {
    case 0x59:
      return '1';
    case 0x5A:
      return '2';
    case 0x5B:
      return '3';
    case 0x5C:
      return '4';
    case 0x5D:
      return '5';
    case 0x5E:
      return '6';
    case 0x5F:
      return '7';
    case 0x60:
      return '8';
    case 0x61:
      return '9';
    case 0x62:
      return '0';
    case 0x63:
      return '.';
    default:
      return 0;
  }
}

bool espUsbHostIsBootKeyboardReportValid(const uint8_t* data, size_t length)
{
  if (!data || length < ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE)
    return false;
  if (data[1] != 0)
    return false;
  for (size_t i = 2; i < ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE; i++)
  {
    if (data[i] >= 0xE0)
      return false;
  }
  return true;
}

size_t espUsbHostUnicodeToUtf8(uint16_t codepoint, char out[4])
{
  out[0] = '\0';
  out[1] = '\0';
  out[2] = '\0';
  out[3] = '\0';

  if (codepoint == 0)
    return 0;

  if (codepoint < 0x80)
  {
    out[0] = static_cast<char>(codepoint);
    return 1;
  }
  if (codepoint < 0x800)
  {
    out[0] = static_cast<char>(0xC0 | (codepoint >> 6));
    out[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
    return 2;
  }
  out[0] = static_cast<char>(0xE0 | (codepoint >> 12));
  out[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
  out[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
  return 3;
}

uint16_t espUsbHostKeycodeToUnicode(uint8_t keycode,
                                    uint8_t modifiers,
                                    EspUsbHostKeyboardLayout layout,
                                    bool capsLock,
                                    bool numLock)
{
  if (keycode >= 128)
    return 0;

  if ((keycode >= 0x54 && keycode <= 0x63) || keycode == 0x67)
    return espUsbHostKeypadKeycodeToAscii(keycode, numLock);

  const uint8_t shift = (modifiers & (MOD_LEFT_SHIFT | MOD_RIGHT_SHIFT)) ? 1 : 0;
  const uint8_t effectiveShift =
      isCapsLockAffected(keycode, layout) ? (shift ^ (capsLock ? 1 : 0)) : shift;

  if (layout == ESP_USB_HOST_KEYBOARD_LAYOUT_UK_UA)
  {
    // AltGr + Г (U, 0x18) -> ґ / Ґ (стандартна комбінація для ANSI-клавіатур)
    if (keycode == 0x18 && (modifiers & MOD_RIGHT_ALT))
      return effectiveShift ? 0x0490 : 0x0491;
    return KEYCODE_TO_UNICODE_UK_UA[keycode][effectiveShift];
  }

  return KEYCODE_TO_ASCII_EN_US[keycode][effectiveShift];
}

uint8_t espUsbHostKeycodeToAscii(uint8_t keycode,
                                 uint8_t modifiers,
                                 EspUsbHostKeyboardLayout layout,
                                 bool capsLock,
                                 bool numLock)
{
  const uint16_t unicode =
      espUsbHostKeycodeToUnicode(keycode, modifiers, layout, capsLock, numLock);
  return unicode < 0x80 ? static_cast<uint8_t>(unicode) : 0;
}

bool espUsbHostParseBootMouseReport(uint8_t interfaceNumber,
                                    const uint8_t* data,
                                    size_t length,
                                    uint8_t previousButtons,
                                    EspUsbHostMouseEvent& event)
{
  if (!data || length < 3)
    return false;

  event = EspUsbHostMouseEvent();
  event.interfaceNumber = interfaceNumber;
  event.previousButtons = previousButtons;
  event.buttons = data[0];
  event.x = static_cast<int8_t>(data[1]);
  event.y = static_cast<int8_t>(data[2]);
  event.wheel = length > 3 ? static_cast<int8_t>(data[3]) : 0;
  event.moved = event.x != 0 || event.y != 0 || event.wheel != 0;
  event.buttonsChanged = event.buttons != event.previousButtons;
  return event.moved || event.buttonsChanged;
}

uint8_t espUsbHostBuildKeyboardLedReport(bool numLock, bool capsLock, bool scrollLock)
{
  uint8_t leds = 0;
  if (numLock)
    leds |= ESP_USB_HOST_KEYBOARD_LED_NUM_LOCK;
  if (capsLock)
    leds |= ESP_USB_HOST_KEYBOARD_LED_CAPS_LOCK;
  if (scrollLock)
    leds |= ESP_USB_HOST_KEYBOARD_LED_SCROLL_LOCK;
  return leds;
}

size_t espUsbHostBuildKeyboardEvents(uint8_t interfaceNumber,
                                     const EspUsbHostKeyboardReport& previousReport,
                                     const EspUsbHostKeyboardReport& currentReport,
                                     EspUsbHostKeyboardLayout layout,
                                     bool capsLock,
                                     bool numLock,
                                     EspUsbHostKeyboardEvent* events,
                                     size_t maxEvents)
{
  if (!events || maxEvents == 0)
    return 0;

  const uint8_t modifiers = currentReport.data[0];
  const uint8_t* keys = currentReport.data + 2;
  const uint8_t* lastKeys = previousReport.data + 2;
  size_t eventCount = 0;

  for (int i = 0; i < 6 && eventCount < maxEvents; i++)
  {
    const uint8_t key = keys[i];
    if (key == 0)
      continue;

    bool existed = false;
    for (int j = 0; j < 6; j++)
    {
      if (lastKeys[j] == key)
      {
        existed = true;
        break;
      }
    }

    if (!existed)
    {
      EspUsbHostKeyboardEvent& event = events[eventCount++];
      event.interfaceNumber = interfaceNumber;
      event.pressed = true;
      event.released = false;
      event.keycode = key;
      event.modifiers = modifiers;
      event.unicode = espUsbHostKeycodeToUnicode(key, modifiers, layout, capsLock, numLock);
      event.ascii = event.unicode < 0x80 ? static_cast<uint8_t>(event.unicode) : 0;
      espUsbHostUnicodeToUtf8(event.unicode, event.utf8);
    }
  }

  for (int i = 0; i < 6 && eventCount < maxEvents; i++)
  {
    const uint8_t key = lastKeys[i];
    if (key == 0)
      continue;

    bool stillPressed = false;
    for (int j = 0; j < 6; j++)
    {
      if (keys[j] == key)
      {
        stillPressed = true;
        break;
      }
    }

    if (!stillPressed)
    {
      EspUsbHostKeyboardEvent& event = events[eventCount++];
      event.interfaceNumber = interfaceNumber;
      event.pressed = false;
      event.released = true;
      event.keycode = key;
      event.modifiers = modifiers;
      event.unicode =
          espUsbHostKeycodeToUnicode(key, previousReport.data[0], layout, capsLock, numLock);
      event.ascii = event.unicode < 0x80 ? static_cast<uint8_t>(event.unicode) : 0;
      espUsbHostUnicodeToUtf8(event.unicode, event.utf8);
    }
  }

  return eventCount;
}

#endif  // #ifndef CONFIG_IDF_TARGET_ESP32
