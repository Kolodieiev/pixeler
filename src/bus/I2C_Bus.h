/**
 * @file I2C_Bus.h
 * @brief Абстакція над бібліотекою I2C
 * @details Загортає стандартні виклики I2C в зручніші методи, задля уникнення дублювання коду.
 */

#pragma once
#pragma GCC optimize("O3")
#include "i2c_config.h"
#include "defines.h"

namespace pixeler
{
  class I2C_Bus
  {
  public:
    enum I2C_MODE : uint8_t
    {
      I2C_MODE_MASTER = 0,
      I2C_MODE_SLAVE
    };

    /**
     * @brief Ініціалізує шину I2C для пристрою.
     *
     * @param mode Режим, в якому буде працювати пристрій.
     * @param slave_addr Адреса веденого пристрою для режиму I2C_MODE_SLAVE.
     * @param receive_callback Обробник вхідних даних для режиму I2C_MODE_SLAVE.
     * @param request_callback Обробник запитів даних для режиму I2C_MODE_SLAVE.
     * @return true - Якщо ініціалізацію виконано успішно.
     * @return false - Інакше.
     */
    bool begin(I2C_MODE mode = I2C_MODE_MASTER, uint8_t slave_addr = 0, void (*receive_callback)(int) = nullptr, void (*request_callback)() = nullptr);

    /**
     * @brief Деініціалізує шину I2C.
     *
     */
    void end();

    /**
     * @brief Перевіряє, чи є з'єднання з I2C-пристроєм за вказаною адресою.
     *
     * @param addr Адреса I2C-пристрою.
     * @return true - Якщо з'єднання наявне.
     * @return false - Інакше.
     */
    bool hasConnect(uint8_t addr) const;

    /**
     * @brief Записує дані до I2C-пристрою за вказаною адресою.
     *
     * @param addr Адреса I2C-пристрою.
     * @param data_buff Буфер з даними.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно записано.
     * @return false - Інакше.
     */
    bool write(uint8_t addr, const void* data_buff, size_t data_size) const;

    /**
     * @brief Записує дані до 8-бітного регістра I2C-пристрою за вказаними адресами.
     *
     * @param addr Адреса I2C-пристрою.
     * @param data_buff Буфер з даними.
     * @param reg Адреса регістра.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно записано.
     * @return false - Інакше.
     */
    bool writeRegister8(uint8_t addr, uint8_t reg, const void* data_buff, size_t data_size) const;

    /**
     * @brief Записує дані до 16-бітного регістра I2C-пристрою за вказаними адресами.
     *
     * @param addr Адреса I2C-пристрою.
     * @param data_buff Буфер з даними.
     * @param reg Адреса регістра.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно записано.
     * @return false - Інакше.
     */
    bool writeRegister16(uint8_t addr, uint16_t reg, const void* data_buff, size_t data_size) const;

    /**
     * @brief Читає дані з 8-бітного регістра I2C-пристрою за вказаними адресами.
     *
     * @param addr Адреса I2C-пристрою.
     * @param reg Адреса регістра.
     * @param out_data_buff Буфер для вхідних даних.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно прочитано.
     * @return false - Інакше.
     */
    bool readRegister8(uint8_t addr, uint8_t reg, void* out_data_buff, uint8_t data_size = 1) const;

    /**
     * @brief Читає дані з 16-бітного регістра I2C-пристрою за вказаними адресами.
     *
     * @param addr Адреса I2C-пристрою.
     * @param reg Адреса регістра.
     * @param out_data_buff Буфер для вхідних даних.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно прочитано.
     * @return false - Інакше.
     */
    bool readRegister16(uint8_t addr, uint16_t reg, void* out_data_buff, uint8_t data_size = 1) const;

    /**
     * @brief Читає дані з I2C-пристрою за вказаною адресою.
     *
     * @param addr Адреса I2C-пристрою.
     * @param out_data_buff Буфер для вхідних даних.
     * @param data_size Розмір даних.
     * @return true - Якщо дані було успішно прочитано.
     * @return false - Інакше.
     */
    bool read(uint8_t addr, void* out_data_buff, uint8_t data_size) const;

    /**
     * @brief Розпочинає безперервне пересилання даних до I2C-пристрою за вказаною адресою.
     *
     * @param addr Адреса I2C-пристрою.
     */
    void beginTransmission(uint8_t addr) const;

    /**
     * @brief Завершує поточне безперервне пересилання даних до I2C-пристрою.
     *
     * @return true - Якщо транзакція виконана успішно.
     * @return false - Інакше.
     */
    bool endTransmission() const;

    /**
     * @brief Надсилає буфер даних до поточної відкритої транзакції з I2C-пристроєм.
     *
     * @param data_buff Буфер з даними.
     * @param data_size Розмір даних.
     * @return true - Якщо всі дані було успішно записано.
     * @return false - Інакше.
     */
    bool send(const void* data_buff, size_t data_size = 1) const;

    /**
     * @brief Читає дані з відкритої транзакції з I2C-пристроєм.
     *
     * @param out_data_buff Буфер, до якого будуть записані прочитані дані.
     * @param data_size Розмір очікуваних даних.
     * @return true - Якщо всі дані було успішно прочитано.
     * @return false - Інакше.
     */
    bool receive(void* out_data_buff, size_t data_size = 1) const;

    /**
     * @brief Повертає стан ініціалізації шини I2C.
     *
     * @return true - Якщо шина була ініціалізована.
     * @return false - Інакше.
     */
    bool isInited() const;

    /**
     * @brief Запускає сканування пристроїв на шині I2C.
     * Адреси виявлених пристроїв буде виведено до терміналу.
     *
     */
    void scanBus() const;

    /**
     * @brief Встановлює розмір буферів шини I2C.
     *
     * @param buffer_size
     */
    void setBufferSize(size_t buffer_size);

    I2C_Bus() {}

    I2C_Bus(const I2C_Bus&) = delete;
    I2C_Bus& operator=(const I2C_Bus&) = delete;

    I2C_Bus(I2C_Bus&&) = delete;
    I2C_Bus& operator=(I2C_Bus&&) = delete;

  private:
    bool _is_inited = false;
  };

  /**
   * @brief Глобальний об'єкт-обгортка для роботи з шиною I2C.
   *
   */
  extern I2C_Bus _i2c;
}  // namespace pixeler