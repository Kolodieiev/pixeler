/**
 * @file DisplayWrapper.h
 * @brief Абстракція над Arduino_GFX бібліотекою
 * @details Розширює можливості Arduino_GFX такими функціями, як подвійна буферизація, обертання зображень та ін.
 * Не повинен реалізовуватися в користувацьому коді.
 * Доступний для використання через глобальний об'єкт "_display" в класі контексту.
 *
 * Налаштовується в config/graphics_config.h
 */

#pragma once
#pragma GCC optimize("O3")
#include <stdint.h>

#include "Arduino_GFX/Arduino_GFX_Library.h"
#include "graphics_config.h"

#define WDT_GUARD_TIME 1000UL
#define PPA_FILL_SIZE_TRIGG 67599U
#define PPA_IMG_SIZE_TRIGG 51983U

namespace pixeler
{
  class DisplayWrapper
  {
  public:
    enum RotateAngle : uint8_t
    {
      ROTATE_ANGLE_90 = 0,
      ROTATE_ANGLE_180,
      ROTATE_ANGLE_270
    };

    DisplayWrapper();
    ~DisplayWrapper();

    /**
     * @brief Ініціалізує драйвер графіки відповідно до налаштувань.
     * Не потрібно викликати метод самостійно.
     *
     */
    void __init();

#ifndef DIRECT_DRAWING
    /**
     * @brief Надсилає буфер канвасу до дисплея.
     *
     * Майже ніколи не потрібно викликати метод самостійно.
     *
     */
    void __flush();
#endif  // #ifndef DIRECT_DRAWING

    /**
     * @brief Заливає канвас вказаним кольором.
     *
     * @param color
     */
    void fillScreen(uint16_t color);

    /**
     * @brief Встановлює поточний шрифт, який буде використовуватись для виводу тексту в канвас.
     *
     * @param font
     */
    void setFont(const uint8_t* font);

    /**
     * @brief Встановлює множник до початкового розміру шрифту.
     *
     * @param size
     */
    void setTextSize(uint8_t size);

    /**
     * @brief Встановлює колір тексту.
     *
     * @param color
     */
    void setTextColor(uint16_t color);

    /**
     * @brief Встановлює позицію курсору, з якої буде виводитись текст.
     *
     * @param x
     * @param y
     */
    void setCursor(int16_t x, int16_t y);

    /**
     * @brief Встановлює прапор, який вказує, чи повинен переноситися текст на новий рядок,
     * якщо досягнуто кінця канвасу в поточному рядку.
     *
     * @param state Якщо true - текст буде автоматично переноситись. false - текст буде обрізано.
     */
    void setTextWrap(bool state);

    /**
     * @brief Встановлює поле, яке обмежує собою місце для виведення тексту на дисплей.
     *
     * @param x Координата в пікселях.
     * @param y Координата в пікселях.
     * @param w Ширина рамки.
     * @param h Висота рамки.
     */
    void setTextBound(int16_t x, int16_t y, int16_t w, int16_t h);

    /**
     * @brief Прибирає поле, яке встановлює кордони для виводу текста.
     *
     */
    void resetTextBound();

    /**
     * @brief Виводить текст з поточної позиції курсора на канвас.
     *
     * @param str Текст, який необхідно вивести.
     * @return size_t - Кількість виведених байтів.
     */
    size_t print(const char* str);

    /**
     * @brief Має ефект, тільки якщо підтримується на МК.
     * Встановлює стан активності модуля апаратного прискорення для операцій заповнення кольором
     * та копіювання зображень в буфер кадру.
     * НЕ ЕФЕКТИВНЕ ДЛЯ МАЛИХ БЛОКІВ.
     * @param state
     */
    void setPPAState(bool state);

    /**
     * @brief Повертає стан активності PPA модуля.
     *
     * @return true - Якщо PPA увімкнено.
     * @return false - Інакше.
     */
    bool isPPAEnabled() const;

    /**
     * @brief Розраховує межі, до яких займатиме місце на канвасі вказаний текст.
     *
     * @param str Текст, для якого необхідно виконати розрахунок.
     * @param x Початкова позиція курсора по координаті X.
     * @param y Початкова позиція курсора по координаті Y.
     * @param[out] x_out Початкова позиція, куди буде встановлено курсор для виводу тексту, по координаті X.
     * @param[out] y_out Початкова позиція, куди буде встановлено курсор для виводу тексту, по координаті Y.
     * @param[out] w_out Кількість пікселів, яку займатиме текст по ширині.
     * @param[out] h_out Кількість пікселів, яку займатиме текст по висоті.
     */
    void calcTextBounds(const char* str, int16_t x, int16_t y, int16_t& x_out, int16_t& y_out, uint16_t& w_out, uint16_t& h_out);

    /**
     * @brief Встановлює колір пікселя на канвасі.
     *
     * @param x X-координата пікселя для якого необхідно встановити колір.
     * @param y Y-координата пікселя для якого необхідно встановити колір.
     * @param color Колір пікселя.
     */
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    /**
     * @brief Малює пряму лінію на канвасі.
     *
     * @param x0 Початкова х-координата лінії.
     * @param y0 Початкова у-координата лінії.
     * @param x1 Кінцева х-координата лінії.
     * @param y1 Кінцева у-координата лінії.
     * @param color Колір лінії.
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

    /**
     * @brief Малює коло на канвасі.
     *
     * @param x X-координата центру кола.
     * @param y Y-координата центру кола.
     * @param r Радіус кола.
     * @param color Колір кола.
     */
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);

    /**
     * @brief Заливає площу на канвасі, що описана колом.
     *
     * @param x X-координата центру кола.
     * @param y Y-координата центру кола.
     * @param r Радіус кола.
     * @param color Колір заливання.
     */
    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);

    /**
     * @brief Малює прямокутник на канвасі.
     *
     * @param x X-координата верхнього лівого кута прямокутника.
     * @param y Y-координата верхнього лівого кута прямокутника.
     * @param w Ширина прямокутника.
     * @param h Висота прямокутника.
     * @param color Колір ліній.
     */
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);

    /**
     * @brief Заливає площу на канвасі, що описана прямокутником.
     *
     * @param x X-координата верхнього лівого кута прямокутника.
     * @param y Y-координата верхнього лівого кута прямокутника.
     * @param w Ширина прямокутника.
     * @param h Висота прямокутника.
     * @param color Колір заливання.
     */
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);

    /**
     * @brief Малює прямокутник зі зкругленими кутами на канвасі.
     *
     * @param x X-координата верхнього лівого кута прямокутника.
     * @param y Y-координата верхнього лівого кута прямокутника.
     * @param w Ширина прямокутника.
     * @param h Висота прямокутника.
     * @param radius Радіус кола, по якому буде зкруглено кути прямокутника.
     * @param color Колір ліній.
     */
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color);

    /**
     * @brief Заливає площу на канвасі, що описана прямокутником зі зкругленими кутами.
     *
     * @param x X-координата верхнього лівого кута прямокутника.
     * @param y Y-координата верхнього лівого кута прямокутника.
     * @param w Ширина прямокутника.
     * @param h Висота прямокутника.
     * @param radius Радіус кола, по якому буде зкруглено кути прямокутника.
     * @param color Колір заливання.
     */
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint16_t color);

    /**
     * @brief Виводить вміст буферу на канвас у вигляді зображення без псевдопрозорості.
     *
     * @param x X-координата верхнього лівого кута зображення.
     * @param y Y-координата верхнього лівого кута зображення.
     * @param bitmap Вказівник на буфер зображення.
     * @param w Ширина зображення.
     * @param h Висота зображення.
     */
    void drawBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);

    /**
     * @brief Виводить вміст буферу на канвас у вигляді зображення з ігноруванням кольору COLOR_TRANSPARENT.
     *
     * @param x X-координата верхнього лівого кута зображення.
     * @param y Y-координата верхнього лівого кута зображення.
     * @param bitmap Вказівник на буфер зображення.
     * @param w Ширина зображення.
     * @param h Висота зображення.
     */
    void drawBitmapTransp(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);

    /**
     * @brief Виводить повернутий вміст буферу на канвас у вигляді зображення з ігноруванням кольору COLOR_TRANSPARENT.
     *
     * @param x X-координата верхнього лівого кута зображення.
     * @param y Y-координата верхнього лівого кута зображення.
     * @param bitmap Вказівник на буфер зображення.
     * @param w Ширина зображення.
     * @param h Висота зображення.
     * @param piv_x X-координата точки обертання зображення.
     * @param piv_y Y-координата точки обертання зображення.
     * @param angle Кут повороту.
     */
    void drawBitmapRotated(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h, float piv_x, float piv_y, float angle);

    /**
     * @brief Повертає висоту вказаного шрифту в пікселях.
     *
     * @param font Вказівник на шрифт.
     * @param size Множник розміру шрифту.
     * @return uint16_t
     */
    uint16_t getFontHeight(const uint8_t* font, uint8_t size = 1);

    /**
     * @brief Повертає мютекс, який захищає буфер кадру та вивід зображення по SPI.
     *
     * @return SemaphoreHandle_t - якщо драйвер ініціалізовано в режимі буферизованого виводу зображення.
     * @return nullptr - інакше.
     */
    SemaphoreHandle_t getMutex();

    /**
     * @brief Розвертає в буфері зображення частину, що обмежена заданим квадратом.
     *
     * @param x Кородината верхнього лівого кута квадрата.
     * @param y Кородината верхнього лівого кута квадрата.
     * @param side_len Довжина сторони квадрата.
     * @param angle ROTATE_ANGLE_90/ROTATE_ANGLE_180/ROTATE_ANGLE_270.
     */
    void rotateDisplaySquare(uint16_t x, uint16_t y, uint16_t side_len, RotateAngle angle);

#ifdef ENABLE_SCREENSHOTER
    void takeScreenshot();
#endif  // ENABLE_SCREENSHOTER

#ifdef PIN_DISPLAY_BL
    /**
     * @brief Вмикає підсвітку дисплея зі 100% яскравістю.
     *
     */
    void enableBackLight();

    /**
     * @brief Вимикає підсвітку дисплея
     *
     */
    void disableBackLight();

#ifdef HAS_BL_PWM
    /**
     * @brief Встановлює яскравість підсвітки дисплея.
     * Де 0 - підсвітка вимкнена, а 255 - рівень яскравості максимальний.
     *
     * @param value
     */
    void setBrightness(uint8_t value);

    /**
     * @brief Повертає значення поточної яскравості підсвітки дисплея.
     *
     * @return uint8_t
     */
    uint8_t getBrightness() const;
#endif  // HAS_BL_PWM
#endif  // PIN_DISPLAY_BL

  private:
#ifdef DOUBLE_BUFFERRING
    static void displayRendererTask(void* params);
#endif  // DOUBLE_BUFFERRING

#ifdef ENABLE_SCREENSHOTER
    static void takeScreenshot(DisplayWrapper* self);
#endif  // ENABLE_SCREENSHOTER

  private:
#ifdef GRAPHICS_ENABLED
    BUS_TYPE _bus{BUS_PARAMS};
    Arduino_GFX* _output = new DISP_DRIVER_TYPE(&_bus, DISP_DRIVER_PARAMS);
#ifndef DIRECT_DRAWING
    Arduino_Canvas _canvas{UI_WIDTH, UI_HEIGHT, _output};
#endif  // #ifndef DIRECT_DRAWING
#endif  // #ifdef GRAPHICS_ENABLED

    volatile xSemaphoreHandle _sync_mutex{nullptr};

#ifdef SHOW_FPS
    uint64_t _frame_timer{0};
    uint16_t _frame_counter{0};
    uint16_t _temp_frame_counter{0};
#endif  // SHOW_FPS

#ifdef DOUBLE_BUFFERRING
    volatile bool _has_frame{false};
#endif  // DOUBLE_BUFFERRING

    volatile bool _take_screenshot{false};
    bool _is_buff_changed = false;

#ifdef HAS_BL_PWM
    uint8_t _cur_brightness{255};
#endif  // HAS_BL_PWM
  };

  /**
   * @brief Глобальний об'єкт для взаємодії з драйвером графіки.
   *
   */
  extern DisplayWrapper _display;
}  // namespace pixeler
