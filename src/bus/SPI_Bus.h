/**
 * @file SPI_Bus.h
 * @brief Менеджер ініціалізції та деініціалізації пристроїв SPI
 * @details Ініціалізує та деініціалізує пристрої SPI. 
 * Слідкує, щоб не виникало повторної ініціалізації раніше ініціалізованої шини.
 */

#pragma once
#pragma GCC optimize("O3")
#include <SPI.h>

#include <unordered_map>

#include "defines.h"

namespace pixeler
{
  class SPI_Bus
  {
  public:
    /**
     * @brief Ініціалізує шину SPI.
     *
     * @param bus_num Номер модуля SPI. HSPI/VSPI тощо.
     * @param sclk_pin Номер піна CLK.
     * @param miso_pin Номер піна MISO.
     * @param mosi_pin Номер піна MOSI.
     * @return true - Якщо шину було успішно ініціалізовано.
     * @return false - Інакше.
     */
    static bool init(uint8_t bus_num, int8_t sclk_pin = -1, int8_t miso_pin = -1, int8_t mosi_pin = -1);

    /**
     * @brief Деініціалізує шину для вказаного модуля SPI, якщо її було ініціалізовано раніше.
     *
     * @param bus_num Номер модуля SPI. HSPI/VSPI тощо.
     */
    static void deinitBus(uint8_t bus_num);

    /**
     * @brief Повертає вказівник на ініціалізовану раніше шину SPI для вказаного модуля.
     *
     * @param bus_num Номер модуля SPI. HSPI/VSPI тощо.
     * @return SPIClass* - Якщо шину для вказаного модуля було ініціалізовано раніше.
     * @return nullptr - Інакше.
     */
    static SPIClass* getSpi4Bus(uint8_t bus_num);

    SPI_Bus(const SPI_Bus&) = delete;
    SPI_Bus& operator=(const SPI_Bus&) = delete;
    SPI_Bus(SPI_Bus&&) = delete;
    SPI_Bus& operator=(SPI_Bus&&) = delete;

  private:
    SPI_Bus() {}
    static std::unordered_map<uint8_t, SPIClass*> _spi_map;
  };
}  // namespace pixeler
