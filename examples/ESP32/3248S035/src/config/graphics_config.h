/**
 * @file graphics_config.h
 * @brief Файл налаштувань графічного драйвера
 * @details
 */

#pragma once

#define GRAPHICS_ENABLED  // Увімкнути підтримку графічного драйвера. Закоментуй, якщо в проєкті не використовується дисплей.
#define DIRECT_DRAWING    // Увімкнути пряме малювання на дисплей замість буферу. Закоментуй, для формування зображення через буфер.

#ifdef GRAPHICS_ENABLED

#define UI_UPDATE_DELAY 25  // Затримка (мс) між фреймами

#ifndef DIRECT_DRAWING
#define DOUBLE_BUFFERRING  // Подвійна буферизація. Працює тільки за наявності PSRAM. Інакше буде викликано рестарт МК
// #define SHOW_FPS             // Відображати значення FPS на дисплеї
// #define ENABLE_SCREENSHOTER  // Увімкнути підтримку створення скриншотів. Тримай закоментованим, якщо не використовується
#endif  // #ifndef DIRECT_DRAWING

//-----------------------------------------------------------------------------------------------------------------------------

#define BUSS_FREQUENCY 80000000

#define UI_WIDTH 320   // Ширина UI(канвасу).
#define UI_HEIGHT 480  // Висота UI(канвасу).

#define ROTATE_CANVAS false  // Зазвичай не потрібно встановлювати в true. Можна спробувати, якщо нічого не допомогло.

#define DISPLAY_WIDTH 320   // Ширина дисплея.
#define DISPLAY_HEIGHT 480  // Висота дисплея.

#define SPI_PORT HSPI  // Порт, на якому працюватиме шина SPI дисплея.
//
#define TFT_MOSI 13  // sda.
#define TFT_SCLK 14  // scl.
#define TFT_MISO 12  // Навіть якщо на дисплеї відсутній цей пін, його не можна ніде використовувати для коректної роботи шини SPI.
#define TFT_RST -1   // Якщо пін підключено RST мікроконтролера, вказати -1
#define TFT_DC 2     //
#define TFT_CS 15    // Якщо на шині SPI знаходиться тільки дисплей(що рекомендовано), вказати -1

#define PIN_DISPLAY_BL 27         // Закоментуй, якщо відсутній пін управління підсвіткою дисплея.
#define HAS_BL_PWM                // Закоментуй, якщо відсутнє управління яскравістю підсвітки дисплея.
#define DISPLAY_BL_PWM_FREQ 3000  // Частота PWM підсвітки дисплея.
#define DISPLAY_BL_PWM_RES 8      //

#define DISPLAY_ROTATION 0  // Стартова орієнтація дисплея.

#define IS_IPS_DISPLAY true  // Тип матриці дисплея.
#define INVERT_COLORS true   // Чи потрібно інвертувати кольори пікселів.

#define BUS_TYPE Arduino_ESP32SPI                                                         // Клас шини.
#define IS_COMMON_BUS false                                                               // Парапор, який вказує чи є шина спільною для декількох пристроїв.
#define BUS_PARAMS TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, SPI_PORT, IS_COMMON_BUS  // Параметри класу шини.

#define DISP_DRIVER_TYPE Arduino_ST7796                                                              // Клас драйвера дисплея.
#define DISP_DRIVER_PARAMS TFT_RST, DISPLAY_ROTATION, IS_IPS_DISPLAY, DISPLAY_WIDTH, DISPLAY_HEIGHT  // Параметри класу драйвера дисплея БЕЗ адреси шини.

#endif  // #ifdef GRAPHICS_ENABLED
