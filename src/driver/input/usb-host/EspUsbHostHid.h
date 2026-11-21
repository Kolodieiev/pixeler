#ifndef ESP_USB_HOST_HID_H
#define ESP_USB_HOST_HID_H

#include <sdkconfig.h>

#ifndef CONFIG_IDF_TARGET_ESP32

#include <Arduino.h>

#include "EspUsbHost.h"

static constexpr size_t ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE = 8;
static constexpr size_t ESP_USB_HOST_BOOT_KEYBOARD_MAX_EVENTS = 12;

struct EspUsbHostKeyboardReport
{
  uint8_t data[ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE] = {};
};

bool espUsbHostParseBootMouseReport(uint8_t interfaceNumber,
                                    const uint8_t* data,
                                    size_t length,
                                    uint8_t previousButtons,
                                    EspUsbHostMouseEvent& event);

uint8_t espUsbHostBuildKeyboardLedReport(bool numLock, bool capsLock, bool scrollLock);

size_t espUsbHostBuildKeyboardEvents(uint8_t interfaceNumber,
                                     const EspUsbHostKeyboardReport& previousReport,
                                     const EspUsbHostKeyboardReport& currentReport,
                                     EspUsbHostKeyboardLayout layout,
                                     bool capsLock,
                                     bool numLock,
                                     EspUsbHostKeyboardEvent* events,
                                     size_t maxEvents);

bool espUsbHostIsBootKeyboardReportValid(const uint8_t* data, size_t length);
uint16_t espUsbHostKeycodeToUnicode(uint8_t keycode,
                                    uint8_t modifiers,
                                    EspUsbHostKeyboardLayout layout,
                                    bool capsLock,
                                    bool numLock);
uint8_t espUsbHostKeycodeToAscii(uint8_t keycode,
                                 uint8_t modifiers,
                                 EspUsbHostKeyboardLayout layout,
                                 bool capsLock,
                                 bool numLock);
uint8_t espUsbHostKeypadKeycodeToAscii(uint8_t keycode, bool numLock);
size_t espUsbHostUnicodeToUtf8(uint16_t codepoint, char out[4]);

#endif  // #ifndef CONFIG_IDF_TARGET_ESP32
#endif  // ESP_USB_HOST_HID_H
