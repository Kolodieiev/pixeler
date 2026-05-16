/**
 * @file i2s_config.h
 * @brief Файл налаштувань шини I2S
 * @details Якщо цифровий мікрофон або звукову карту не підключено, визначення пінів можна залишити як є.
 */

#pragma once
#include <stdint.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)

// Піни до яких підключено модуль звукової карти.
#define PIN_I2S_OUT_BCLK 42
#define PIN_I2S_OUT_LRC 1
#define PIN_I2S_OUT_DOUT 2

// Піни до яких підключено цифровий мікрофон.
#define PIN_I2S_IN_SCK 3
#define PIN_I2S_IN_WS 46
#define PIN_I2S_IN_SD 9

#else

#define PIN_I2S_OUT_DOUT 9
#define PIN_I2S_OUT_LRC 10
#define PIN_I2S_OUT_BCLK 13

#define PIN_I2S_IN_SD 12
#define PIN_I2S_IN_SCK 13
#define PIN_I2S_IN_WS 10

#endif