/**
 * @file graphics_config.h
 * @brief Файл налаштувань графічного драйвера
 * @details
 */

#pragma once

// #define GRAPHICS_ENABLED  // Увімкнути підтримку графічного драйвера. Закоментуй, якщо в проєкті не використовується дисплей.

#define UI_UPDATE_DELAY 25  // Мінімальна затримка (мс) між фреймами(викликом update).

#define UI_WIDTH 1024  // Ширина UI.
#define UI_HEIGHT 600  // Висота UI.

#define ROTATE_CANVAS false  // Зазвичай не потрібно встановлювати в true. Можна спробувати, якщо нічого не допомогло.

#define DISPLAY_WIDTH 1024  // Ширина дисплея.
#define DISPLAY_HEIGHT 600  // Висота дисплея.

#ifdef GRAPHICS_ENABLED

// #define SHOW_FPS  // Відображати значення FPS на дисплеї.
// #define ENABLE_SCREENSHOTER  // Увімкнути підтримку створення скриншотів. Тримай закоментованим, якщо не використовується.

//-----------------------------------------------------------------------------------------------------------------------------

#define LCD_DRIVER_EK79007  // Для якого драйвера буде застосовано команду ініціалізації дисплея.
// #define LCD_DRIVER_ST7701
// #define LCD_DRIVER_HX8394
// #define LCD_DRIVER_JD9165
// #define LCD_DRIVER_JD9365

#define HSYNC_PULSE_WIDTH 10
#define HSYNC_BACK_PORCH 160
#define HSYNC_FRONT_PORCH 160

#define VSYNC_PULSE_WIDTH 1
#define VSYNC_BACK_PORCH 23
#define VSYNC_FRONT_PORCH 12

#define BUSS_FREQUENCY 60000000
#define LANE_BITRATE 1200

#define CLOCK_SOURCE MIPI_DSI_PHY_PLLREF_CLK_SRC_PLL_F20M  // Для ревізії чіпа P4 3+ Потрібно задати інше джерело тактування

#define PIN_LCD_RESET 27

#define PIN_DISPLAY_BL 26         // Закоментуй, якщо відсутній пін управління підсвіткою дисплея.
#define HAS_BL_PWM                // Закоментуй, якщо відсутнє управління яскравістю підсвітки дисплея.
#define DISPLAY_BL_PWM_FREQ 3000  // Частота PWM підсвітки дисплея.
#define DISPLAY_BL_PWM_RES 8

#define DISPLAY_ROTATION 0  // Стартова орієнтація дисплея.

#define BUS_TYPE Arduino_ESP32DSIPanel                                                                                                                                         // Клас шини.
#define BUS_PARAMS HSYNC_PULSE_WIDTH, HSYNC_BACK_PORCH, HSYNC_FRONT_PORCH, VSYNC_PULSE_WIDTH, VSYNC_BACK_PORCH, VSYNC_FRONT_PORCH, BUSS_FREQUENCY, LANE_BITRATE, CLOCK_SOURCE  // Параметри класу шини.

#define DISP_DRIVER_TYPE Arduino_DSI_Display                                               // Клас драйвера дисплея.
#define DISP_DRIVER_PARAMS DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_ROTATION, PIN_LCD_RESET  // Параметри класу драйвера дисплея БЕЗ адреси шини.

#endif  // #ifdef GRAPHICS_ENABLED
