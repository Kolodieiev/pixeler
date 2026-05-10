#pragma GCC optimize("O3")
#include "SPI_Bus.h"

namespace pixeler
{
  std::unordered_map<uint8_t, SPIClass*> SPI_Bus::_spi_map;

  bool SPI_Bus::init(uint8_t bus_num, int8_t sclk_pin, int8_t miso_pin, int8_t mosi_pin)
  {
    auto it = _spi_map.find(bus_num);
    if (it != _spi_map.end())
      return true;

    SPIClass* spi = nullptr;
    try
    {
      spi = new SPIClass(bus_num);
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }

    if (!spi->begin(sclk_pin, miso_pin, mosi_pin))
    {
      log_i("Помилка ініціалізації SPI для порту %u. SCLK: %i, MISO: %i, MOSI: %i", bus_num, sclk_pin, miso_pin, mosi_pin);
      return false;
    }
    else
    {
      spiSSDisable(spi->bus());
      spiSSClear(spi->bus());
      _spi_map.insert({bus_num, spi});
      log_i("SPI успішно ініціалізовано для порту %u. SCLK: %i, MISO: %i, MOSI: %i", bus_num, sclk_pin, miso_pin, mosi_pin);
      return true;
    }
  }

  void SPI_Bus::deinitBus(uint8_t bus_num)
  {
    auto it = _spi_map.find(bus_num);
    if (it == _spi_map.end())
      return;

    it->second->end();
    delete (it->second);
    _spi_map.erase(it);
  }

  SPIClass* SPI_Bus::getSpi4Bus(uint8_t bus_num)
  {
    auto it = _spi_map.find(bus_num);

    if (it == _spi_map.end())
      return nullptr;

    return it->second;
  }
}  // namespace pixeler
