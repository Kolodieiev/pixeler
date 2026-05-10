/**
 * @file graphics_config.h
 * @brief Файл налаштувань графічного драйвера
 * @details
 */

#pragma once

#define UI_UPDATE_DELAY 25  // Затримка (мс) між фреймами

#define GRAPHICS_ENABLED  // Увімкнути підтримку графічного драйвера. Закоментуй, якщо в проєкті не використовується дисплей.
// #define DIRECT_DRAWING // Увімкнути пряме малювання на дисплей замість буферу. Закоментуй, для формування зображення через буфер.

#ifdef GRAPHICS_ENABLED

#ifndef DIRECT_DRAWING
#define DOUBLE_BUFFERRING  // Подвійна буферизація. Працює тільки за наявності PSRAM. Інакше буде викликано рестарт МК
// #define SHOW_FPS             // Відображати значення FPS на дисплеї
// #define ENABLE_SCREENSHOTER  // Увімкнути підтримку створення скриншотів. Тримай закоментованим, якщо не використовується
#endif  // #ifndef DIRECT_DRAWING

//-----------------------------------------------------------------------------------------------------------------------------
#define BUSS_FREQUENCY 80000000  // Частота шини дисплея.

#define UI_WIDTH 280   // Ширина UI.
#define UI_HEIGHT 240  // Висота UI.

#define DISPLAY_WIDTH UI_HEIGHT  // Ширина дисплея.
#define DISPLAY_HEIGHT UI_WIDTH  // Висота дисплея.

#define ROTATE_CANVAS false  // Зазвичай не потрібно встановлювати в true. Можна спробувати, якщо нічого не допомогло.

#define SPI_PORT HSPI  // Порт, на якому працюватиме шина SPI дисплея.
//
#define TFT_MOSI 17  // sda.
#define TFT_SCLK 18  // scl.
#define TFT_MISO 8   // Навіть якщо на дисплеї відсутній цей пін, його не можна ніде використовувати для коректної роботи шини SPI.
#define TFT_RST -1   // Якщо пін підключено RST мікроконтролера, вказати -1
#define TFT_DC 15    //
#define TFT_CS 7     // Якщо на шині SPI знаходиться тільки дисплей(що рекомендовано), вказати -1

#define PIN_DISPLAY_BL 46  // Закоментуй, якщо відсутній пін управління підсвіткою дисплея.
// #define HAS_BL_PWM        // Закоментуй, якщо відсутнє управління яскравістю підсвітки дисплея.
#define DISPLAY_BL_PWM_FREQ 3000  // Частота PWM підсвітки дисплея.
#define DISPLAY_BL_PWM_RES 8      //

#define DISPLAY_ROTATION 3  // Стартова орієнтація дисплея.

#define IS_IPS_DISPLAY true  // Тип матриці дисплея.
#define INVERT_COLORS false  // Чи потрібно інвертувати кольори пікселів.

#define BUS_TYPE Arduino_ESP32SPI                                                         // Клас шини.
#define IS_COMMON_BUS true                                                                // Парапор, який вказує чи є шина спільною для декількох пристроїв.
#define BUS_PARAMS TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, SPI_PORT, IS_COMMON_BUS  // Параметри класу шини.

#define DISP_DRIVER_TYPE Arduino_ST7789                                                                           // Клас драйвера дисплея.
#define DISP_DRIVER_PARAMS TFT_RST, DISPLAY_ROTATION, IS_IPS_DISPLAY, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 0, 20  // Параметри класу драйвера дисплея БЕЗ адреси шини.

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Нижче налаштування для JC3248W535C

// #define BUSS_FREQUENCY 60000000

// #define UI_WIDTH 320  // Ширина дисплея.
// #define UI_HEIGHT 480 // Висота дисплея.

// #define TFT_CS 45
// #define TFT_SCLK 47
// #define TFT_MOSI 21
// #define TFT_MISO 48
// #define TFT_D2 40
// #define TFT_D3 39
// #define TFT_DC 8
// #define TFT_RST -1

// #define PIN_DISPLAY_BL 1 // Закоментуй, якщо відсутній пін управління підсвіткою дисплея.
// #define HAS_BL_PWM      // Закоментуй, якщо відсутнє управління яскравістю підсвітки дисплея.
// #define DISPLAY_BL_PWM_FREQ 3000   // Частота PWM підсвітки дисплея.
// #define DISPLAY_BL_PWM_RES 8

// #define DISPLAY_ROTATION 0 // Стартова орієнтація дисплея.

// #define IS_IPS_DISPLAY true // Тип матриці дисплея.
// #define INVERT_COLORS true // Чи потрібно інвертувати кольори пікселів.

// #define BUS_TYPE Arduino_ESP32QSPI                                                      // Клас шини.
// #define IS_COMMON_BUS false                                                            // Парапор, який вказує чи є шина спільною для декількох пристроїв.
// #define BUS_PARAMS TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, TFT_D2, TFT_D3, IS_COMMON_BUS // Параметри класу шини.

// #define DISP_DRIVER_TYPE Arduino_AXS15231B                                                  // Клас драйвера дисплея.
// #define DISP_DRIVER_PARAMS TFT_RST, DISPLAY_ROTATION, IS_IPS_DISPLAY, UI_WIDTH, UI_HEIGHT // Параметри класу драйвера дисплея БЕЗ адреси шини.

#endif  // #ifdef GRAPHICS_ENABLED
