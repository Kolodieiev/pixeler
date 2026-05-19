/**
 * @file i2c_config.h
 * @brief Файл налаштувань шини I2C
 * @details
 */

#pragma once
#include <stdint.h>

#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
// #define PIN_I2C_INT 42  // Пін для двостороннього зв'язку між I2C-пристроями, якщо такий наявний.

#define I2C_AWAIT_TIME_MS (unsigned long)100  // Час очікування відповіді від i2c пристрою
