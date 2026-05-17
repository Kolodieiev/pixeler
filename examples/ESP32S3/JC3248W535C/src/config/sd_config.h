/**
 * @file sd_config.h
 * @brief Файл налаштування карти пам'яті
 * @details
 */

#pragma once
#include <stdint.h>

#define SD_MOUNTPOINT "/sd"  // Точка монтування. Не рекомендуєтсья міняти.
#define SD_MAX_FILES 10      // Максимальна кількість одночасно відкритих файлів.
// #define SD_PIN_PWR_ON 45     // Пін вмикання карти пам'яті. Якщо визначено. Інакше закоментуй.
#define SD_PWR_ON_LVL LOW  // Рівень, що потрібно подати на SD_PIN_PWR_ON для вмикання карти.

#define SD_TYPE_SPI  // Розкоментуй один із варіантів підтримуваних інтерфейсів
// #define SD_TYPE_MMC

#ifdef SD_TYPE_SPI

#define SDSPI_BUS HSPI   // Номер шини SPI
#define SDSPI_PIN_CS 10  // Бібліотека карти пам'яті вимагає реальний пін, навіть у випадку, якщо CS приєдано до землі.
#define SDSPI_PIN_MOSI 11
#define SDSPI_PIN_SCLK 12
#define SDSPI_PIN_MISO 13
#define SDSPI_FREQUENCY 80000000  // Частота шини SPI.

#else  // ifdef SD_TYPE_MMC

#define SDMMC_FREQ (40000)              // або 20000, якщо з 40к працює нестабільно
#define SDMMC_SLOT (SDMMC_HOST_SLOT_0)  // або SDMMC_HOST_SLOT_1 якщо P4 і потрібно налаштувати через матрицю

// На P4 для SDMMC_HOST_SLOT_0 всі піни можна встановити в GPIO_NUM_NC
// Їх значення будуть взяті по замовченню для цього слота

#define SDMMC_PIN_CLK 43
#define SDMMC_PIN_CMD 44
#define SDMMC_PIN_D0 39

#define SDMMC_PIN_D1 40  // GPIO_NUM_NC, якщо шина 1-бітна
#define SDMMC_PIN_D2 41  // GPIO_NUM_NC, якщо шина 1-бітна
#define SDMMC_PIN_D3 42  // GPIO_NUM_NC, якщо шина 1-бітна

// #define SDMMC_MODE_1_BIT  // Тримай закоментованим, якщо шина 4-бітна

#define SDMMC_POWER_CHANNEL (BOARD_SDMMC_POWER_CHANNEL)  // BOARD_SDMMC_POWER_CHANNEL для P4. GPIO_NUM_NC, якщо живлення зовнішнє.

#endif  // #ifdef SD_TYPE_MMC
