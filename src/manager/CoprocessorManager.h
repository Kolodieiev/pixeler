/**
 * @file CoprocessorManager.h
 * @brief Драйвер для взаємодії з допоміжним мікроконтролером через I2C
 * @details Забезпечує керування зв'язком між основним та допоміжним МК:
 * - Встановлення та відновлення з'єднання (connect/reconnect)
 * - Передача даних до співпроцесора (send)
 * - Отримання даних від співпроцесора (read)
 * - Обробка помилок комунікації
 *
 *  Використовує протокол I2C для обміну даними.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../defines.h"
#include "coprocessor_config.h"

namespace pixeler
{
#ifdef HAS_COPROCESSOR
#if !defined(COPROCESSOR_ADDR) || (COPROCESSOR_ADDR < 1) || (COPROCESSOR_ADDR > 126)
#error "COPROCESSOR_ADDR не задано, або значення не є коректним."
#endif  // #if !defined(COPROCESSOR_ADDR)

  class CoprocessorManager
  {
  public:
    /**
     * @brief Встановлює з'єднання з допоміжним МК по шині I2C.
     *
     * @return true - Якщо шину було успішно ініціалізовано та з'єднання з допоміжним МК наявне.
     * @return false - Інакше.
     */
    bool connect();

    /**
     * @brief Завершує поточне з'єднання з допоміжним МК. Та намагається встановити нове.
     *
     * @return true - Якщо з'єднання успішно встановлено.
     * @return false - Інакше.
     */
    bool reconnect();

    /**
     * @brief Надсилає команду та дані допоміжному МК по шині I2C.
     *
     * @param cmd_data Вказівник на буфер з командою і даними.
     * @param data_size Розмір буфера.
     * @param resp_delay Затримка після надсилання команди.
     * @return true - Якщо команду було успішно надіслано.
     * @return false - Інакше.
     */
    bool sendCmd(const void* cmd_data, size_t data_size, uint8_t resp_delay = 0) const;

    /**
     * @brief Зчитує дані з допоміжного МК по шині I2C.
     *
     * @param out_buf Вказівник на буфер, куди будуть записані вхідні дані.
     * @param data_size Розмір даних, які необхідно прочитати.
     * @return true - Якщо було успішно отримано вказану кількість байтів.
     * @return false - Інакше.
     */
    bool readData(void* out_buf, size_t data_size) const;

  private:
    bool _is_connected = false;
  };

  /**
   * @brief Глобальний об'єкт для управління допоміжним МК по шині I2C.
   *
   */
  extern CoprocessorManager _ccpu;

#endif  // HAS_COPROCESSOR
}  // namespace pixeler
