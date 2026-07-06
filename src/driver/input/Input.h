/**
 * @file Input.h
 * @brief Реалізація системи обробки користувацького вводу
 * @details Об'єднує та уніфікує обробку вводу з сенсорних датчиків
 * і фізичних пінів ESP або взаємодіє з іншим МК для розширення GPIO.
 *
 * Налаштовується в config/input_config.h
 */

#pragma once
#pragma GCC optimize("O3")

#include <unordered_map>

#include "Button.h"
#include "defines.h"
#include "external_input/ExtInput.h"
#include "input_config.h"
#ifdef TOUCHSCREEN_SUPPORT
#include "ITouchscreen.h"
#endif  // #ifdef TOUCHSCREEN_SUPPORT
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
#include "usb-host/EspUsbHost.h"
#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

namespace pixeler
{
  class Input
  {
  public:
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
    using KeyPressedHandler = std::function<void(const EspUsbHostKeyboardEvent& event, void* arg)>;
    using KeyReleasedHandler = std::function<void(const EspUsbHostKeyboardEvent& event, void* arg)>;

    enum KeyCode : uint8_t
    {
      KEY_UP_ARROW = 0x52,
      KEY_DOWN_ARROW = 0x51,
      KEY_LEFT_ARROW = 0x50,
      KEY_RIGHT_ARROW = 0x4F,
      KEY_PRINT_SCREEN = 0x46,
      KEY_ENTER = 0x28,
      KEY_ENTER_KP = 0x58,
      KEY_ESCAPE = 0x29,
      KEY_BACKSPACE = 0x2A,
      KEY_TAB = 0x2B,
    };

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

    Input();

    /**
     * @brief Ініціалізує драйвер вводу відповідно до налаштувань. Не потрібно викликати метод самостійно.
     *
     */
    void __init();

    /**
     * @brief Оновлює стан вводу. Не потрібно викликати метод самостійно.
     *
     */
    void __update();

    /**
     * @brief Скидає стан вводу. Метод викликається контекстом перед його першим оновленням, щоб уникнути захоплення стану вводу з попереднього контексту.
     * Ручний виклик цього майже ніколи не потрібний.
     *
     */
    void reset();

    /**
     * @brief Вмикає фізичний пін та ініціалізує його в тому режимі, який було передано в конструктор під час створення об'єкта віртуального піна з цим номером.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     * @param btn_id Номер віртуального піна.
     */
    void enableBtn(BtnID btn_id);

    /**
     * @brief Вимикає пін, переводить його в режим високоімпедансного входу, та скидає стани віртуального піна з цим номером.
     * Це може бути корисним, якщо пін не використовується взагалі в поточному контексті або не використовуєтсья тривалий час.
     * Вимкнення піна на тривалий період, може трохи скоротити споживання струму мікроконтролером.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     * @param btn_id Номер віртуального піна.
     */
    void disableBtn(BtnID btn_id);

    /**
     * @brief Перевіряє чи знаходиться зараз віртуальний пін з таким номер в активному стані.
     * Тобто чи натиснута кнопка або чи фіксується дотик на цьому піні.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     *
     * @param btn_id Номер віртуального піна.
     * @return true - Якщо пін утримується.
     * @return false - Інакше.
     */
    bool isHolded(BtnID btn_id) const;

    /**
     * @brief Перевіряє чи утримується пін більше n мілісекунд, що задано в налаштуваннях вводу.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     * @param btn_id Номер віртуального піна.
     * @return true - Якщо пін утримується більше n мілісекунд.
     * @return false - Інакше.
     */
    bool isPressed(BtnID btn_id) const;

    /**
     * @brief Перевіряє чи було пін раніше активовано натисканням та відпущено.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     * @param btn_id Номер віртуального піна.
     * @return true - Якщо пін раніше було активовано та відпущено.
     * @return false - Інакше.
     */
    bool isReleased(BtnID btn_id) const;

    /**
     * @brief Блокує віртуальний пін, щоб запобігти випадковим спрацюванням через брязкіт контактів,
     * або щоб задати час очікування до наступного спрацюванням цього піна. Під час виклику скидає стан віртуального піна
     * та блокує його оновлення, доки не сплине час блокування.
     * Якщо віртуальний пін з таким номером відсутній, буде викликано виключення std::out_of_range.
     *
     * @param btn_id Номер віртуального піна.
     * @param lock_duration Час в мілісекундах, на який потрібно заблокувати віртуальний пін.
     */
    void lock(BtnID btn_id, unsigned long lock_duration);

    /**
     * @brief Службовий метод. Може бути використаний виключно для відлагодження. Виводить в консоль режим, в якому знаходиться фізичний пін мікроконтролера.
     *
     * @param pin_id Номер піна мікроконтролера.
     */
    void __printPinMode(BtnID pin_id);

#ifdef TOUCHSCREEN_SUPPORT
    /**
     * @brief Повертає значення, яке вказує, чи фіксується в даний момент дотик до сенсорного екрану.
     *
     * @return true - Якщо на сенсорному екрані зафіксовано дотик.
     * @return false - Інакше.
     */
    bool isHolded() const;

    /**
     * @brief Повертає значення, яке вказує, чи утримується сенсорний екран тривалий час.
     *
     * @return true - Якщо на сенсорному екрані зафіксовано тривалий дотик.
     * @return false - Інакше.
     */
    bool isPressed() const;

    /**
     * @brief Повертає значення, яке вказує, чи було зафіксовано дотик на сенсорному екрані раніше.
     *
     * @return true - Якщо на сенсорному екрані було раніше зафіксовано дотик, але тепер дотик не фіксується.
     * @return false - Інакше.
     */
    bool isReleased() const;

    /**
     * @brief Блокує розпізнавання жестів на сенсорному екрані на вказаний час.
     *
     * @param lock_duration час в мілісекундах, на який буде заблоковано розпізнавання жестів.
     */
    void lock(unsigned long lock_duration);

    /**
     * @brief Повертає останній розпізнаний на сенсорному екрані жест.
     *
     * @return ITouchscreen::Swipe
     */
    ITouchscreen::Swipe getSwipe();

    /**
     * @brief Повертає X-координату останнього дотика, зафіксованого на сенсорному екрані.
     *
     * @return uint16_t
     */
    uint16_t getTouchX() const;

    /**
     * @brief Повертає Y-координату останнього дотика, зафіксованого на сенсорному екрані.
     *
     * @return uint16_t
     */
    uint16_t getTouchY() const;

#endif  // TOUCHSCREEN_SUPPORT

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
    /**
     * @brief Встановлює обробник для події натискання клавіші на фізичній клавіатурі.
     *
     * @param handler Обробник події.
     * @param arg Аргумент, який буде передано обробнику.
     */
    void onKeyPressed(const KeyPressedHandler handler, void* arg = nullptr);

    /**
     * @brief Встановлює обробник для події відтискання клавіші на фізичній клавіатурі.
     *
     * @param handler Обробник події.
     * @param arg Аргумент, який буде передано обробнику.
     */
    void onKeyReleased(const KeyReleasedHandler handler, void* arg = nullptr);

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

  private:
#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
    static void keyEventHandler(const EspUsbHostKeyboardEvent& event, void* arg);

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

    std::unordered_map<BtnID, Button> _buttons BUTTONS_TMPL;

#if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)
    EspUsbHost _usb;
    KeyPressedHandler _key_pressed_handler{nullptr};
    void* _key_pressed_arg{nullptr};
    KeyReleasedHandler _key_released_handler{nullptr};
    void* _key_released_arg{nullptr};

#endif  // #if defined(KEYBOARD_SUPPORT) && !defined(CONFIG_IDF_TARGET_ESP32)

#ifdef EXT_INPUT
    ExtInput _ext_input;
#endif

#ifdef TOUCHSCREEN_SUPPORT
    ITouchscreen* _touchscreen{nullptr};
#endif
  };

  /**
   * @brief Глобальний об'єкт для зчитування стану вводу.
   *
   */
  extern Input _input;
}  // namespace pixeler
