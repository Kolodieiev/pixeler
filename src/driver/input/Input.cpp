#pragma GCC optimize("O3")

#include "Input.h"

#include <stdexcept>

#include "soc/gpio_periph.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"

#ifdef GT911_DRIVER
#include "./touch_driver/GT911.h"
#elifdef AXS15231B_DRIVER
#include "./touch_driver/AXS15231B.h"
#endif

static const char STR_UNKNOWN_PIN[] = "Незареєстрована віртуальна кнопка";

namespace pixeler
{
  Input::Input() : _hold_lock_time{HOLD_LOCK_TIME_MS}, _click_lock_time{CLICK_LOCK_TIME_MS}, _press_lock_time{PRESS_LOCK_TIME_MS}
  {
  }

  void Input::__init()
  {
#ifdef EXT_INPUT
    _ext_input.init();
#endif  // EXT_INPUT

#ifdef TOUCHSCREEN_SUPPORT
#ifdef GT911_DRIVER
    _touchscreen = new GT911();
#elifdef AXS15231B_DRIVER
    _touchscreen = new AXS15231B();
#endif  // GT911_DRIVER
    _touchscreen->__begin();
    _touchscreen->setRotation(ITouchscreen::TOUCH_ROTATION);
#endif  // TOUCHSCREEN_SUPPORT

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
    _usb.setKeyboardLayout(ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US);

    _usb.onDeviceConnected([](const EspUsbHostDeviceInfo& device)
                           { espUsbHostPrint(device); });

    _usb.onDeviceDisconnected([](const EspUsbHostDeviceInfo& device)
                              { espUsbHostPrint(device); });

    _usb.onKeyboard(keyEventHandler, this);

    const EspUsbHostConfig config{
        .taskStackSize = 8 * 1024,
        .taskPriority = 10,
        .taskCore = 1,
        .port = ESP_USB_HOST_PORT_DEFAULT};

    if (!_usb.begin(config))
      log_e("USB-host begin failed: %s", _usb.lastErrorName());
    else
      log_e("USB-host begin successfully");

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
  }

  void Input::__update()
  {
#ifdef TOUCHSCREEN_SUPPORT
    _touchscreen->__update();
#endif  // #ifdef TOUCHSCREEN_SUPPORT

#ifdef EXT_INPUT
    _ext_input.update();

    for (auto&& btn : _buttons)
      btn.second.__extUpdate(_ext_input.getBtnState(btn.first));
#else
    for (auto&& btn : _buttons)
      btn.second.__update();
#endif  // EXT_INPUT
  }

  void Input::reset()
  {
#ifdef TOUCHSCREEN_SUPPORT
    _touchscreen->reset();
#endif  // TOUCHSCREEN_SUPPORT

    for (auto&& btn : _buttons)
      btn.second.reset();
  }

  void Input::__printPinMode(uint16_t pin_id)
  {
    if ((gpio_num_t)pin_id >= GPIO_NUM_MAX)
    {
      log_i("Invalid pin number: %d", pin_id);
      return;
    }

    uint32_t io_mux_reg = GPIO_PIN_MUX_REG[pin_id];  // Отримати адресу IOMUX регістра

    if (REG_GET_BIT(io_mux_reg, FUN_PU))
      log_i("Pin %d: Pull-up enabled", pin_id);
    else if (REG_GET_BIT(io_mux_reg, FUN_PD))
      log_i("Pin %d: Pull-down enabled", pin_id);
    else
      log_i("Pin %d: is floating", pin_id);
  }

  void Input::enableBtn(BtnID btn_id)
  {
    try
    {
#ifdef EXT_INPUT
      _ext_input.enableBtn(btn_id);
#endif  // EXT_INPUT

      _buttons.at(btn_id).enable();
    }
    catch (const std::out_of_range& ignored)
    {
      log_e("%s : id[%u]", STR_UNKNOWN_PIN);
    }
  }

  void Input::disableBtn(BtnID btn_id)
  {
    try
    {
#ifdef EXT_INPUT
      _ext_input.disableBtn(btn_id);
#endif  // EXT_INPUT

      _buttons.at(btn_id).disable();
    }
    catch (const std::out_of_range& ignored)
    {
      log_e("%s : id[%u]", STR_UNKNOWN_PIN);
    }
  }

  bool Input::isHolded(BtnID btn_id)
  {
    try
    {
      bool result = _buttons.at(btn_id).isHolded();

      if (result)
        _buttons.at(btn_id).lock(_hold_lock_time);

      return result;
    }
    catch (const std::out_of_range& ignored)
    {
      log_e("%s : id[%u]", STR_UNKNOWN_PIN);
      return false;
    }
  }

  bool Input::isPressed(BtnID btn_id)
  {
    try
    {
      bool result = _buttons.at(btn_id).isPressed();

      if (result)
        _buttons.at(btn_id).lock(_press_lock_time);

      return result;
    }
    catch (const std::out_of_range& ignored)
    {
      log_e("%s : id[%u]", STR_UNKNOWN_PIN);
      return false;
    }
  }

  bool Input::isReleased(BtnID btn_id)
  {
    try
    {
      bool result = _buttons.at(btn_id).isReleased();

      if (result)
        _buttons.at(btn_id).lock(_click_lock_time);

      return result;
    }
    catch (const std::out_of_range& ignored)
    {
      log_e("%s : id[%u]", STR_UNKNOWN_PIN);
      return false;
    }
  }

  void Input::setHoldLockTime(unsigned long lock_duration_ms)
  {
    _hold_lock_time = lock_duration_ms;
  }

  void Input::setClickLockTime(unsigned long lock_duration_ms)
  {
    _click_lock_time = lock_duration_ms;
  }

  void Input::setPressLockTime(unsigned long lock_duration_ms)
  {
    _press_lock_time = lock_duration_ms;
  }

#ifdef TOUCHSCREEN_SUPPORT
  bool Input::isHolded() const
  {
    return _touchscreen->isHolded();
  }

  bool Input::isPressed() const
  {
    return _touchscreen->isPressed();
  }

  bool Input::isReleased() const
  {
    return _touchscreen->isReleased();
  }

  void Input::lock(unsigned long lock_duration_ms)
  {
    _touchscreen->lock(lock_duration_ms);
  }

  ITouchscreen::Swipe Input::getSwipe()
  {
    return _touchscreen->getSwipe();
  }

  uint16_t Input::getTouchX() const
  {
    return _touchscreen->getTouchX();
  }

  uint16_t Input::getTouchY() const
  {
    return _touchscreen->getTouchY();
  }

#endif  // TOUCHSCREEN_SUPPORT

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

  void Input::onKeyPressed(const KeyPressedHandler handler, void* arg)
  {
    _key_pressed_handler = handler;
    _key_pressed_arg = arg;
  }

  void Input::onKeyReleased(const KeyReleasedHandler handler, void* arg)
  {
    _key_released_handler = handler;
    _key_released_arg = arg;
  }

  void Input::keyEventHandler(const EspUsbHostKeyboardEvent& event, void* arg)
  {
    Input* self = static_cast<Input*>(arg);

    if (event.pressed)
    {
      if (self->_key_pressed_handler)
      {
        self->_key_pressed_handler(event, self->_key_pressed_arg);
      }
    }
    else if (event.released)
    {
      if (self->_key_released_handler)
      {
        self->_key_released_handler(event, self->_key_released_arg);
      }
    }
    else
    {
      log_e("Unknown usb-event");
    }
  }

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

  Input _input;
}  // namespace pixeler
