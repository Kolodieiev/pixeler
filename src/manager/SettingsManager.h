/**
 * @file SettingsManager.h
 * @brief Менеджер збереження та читання налаштувань прошивки
 * @details Зчитує та записує налаштування на карту пам'яті по SPI.
 * Кожне налаштування записується відкритим рядком в окремий файл по його імені в спеціальний каталог на карті пам'яті.
 */

#pragma once
#pragma GCC optimize("O3")
#include "FileManager.h"
#include "defines.h"

namespace pixeler
{
  /**
   * @brief Дозволяє зберігати та завантажувати окремі налаштування до бінарних файлів на карту пам'яті.
   * Бінарні файли з налаштуваннями зберігаються у папці "/.data/preferences"
   *
   */
  class SettingsManager
  {
  public:
    /**
     * @brief Зберігає налаштування до бінарного файлу.
     *
     * @param pref_name Ім'я налаштування
     * @param value Значення налаштування
     * @param subdir Ім'я підкаталогу з налаштуваннями
     * @return true - якщо операція виконана успішно. false - якщо операція завершилася невдачею
     */
    static bool set(const String& pref_name, const String& value, const String& subdir = emptyString);

    /**
     * @brief Читає налаштування з бінарного файлу.
     *
     * @param pref_name Ім'я налаштування
     * @param subdir Ім'я підкаталогу з налаштуваннями
     * @return String - Рядок, що містить дані налаштування, або порожній рядок, якщо файл з налаштуванням не вдалося прочитати
     */
    static String get(const String& pref_name, const String& subdir = emptyString);

    /**
     * @brief Повертає повний шлях до файлу в каталозі налаштувань.
     * Якщо каталог налаштувань не існує, буде виконана спроба його створення.
     *
     * @param pref_name Шлях до файлу з налаштуваннями
     * @param subdir Ім'я підкаталогу з налаштуваннями
     * @return String - Рядок, що містить повний шлях до файлу, або порожній рядок, у разі помилки
     */
    static String getSettingsFilePath(const String& pref_name, const String& subdir = emptyString);

    /**
     * @brief Повертає повний шлях до каталогу налаштувань.
     *
     * @param sub_dirname Ім'я підкаталогу в каталозі налаштувань, якщо потрібно
     * @return String - Рядок, що містить повний шлях до каталогу, або порожній рядок, у разі помилки
     */
    static String getSettingsDirPath(const String& sub_dirname = emptyString);

    /**
     * @brief Завантажує структуру налаштувань з карти пам'яті.
     *
     * @param out_data_struct Вказівник на пам'ять, куди будуть записані дані з файлу.
     * @param data_struct_size Очікуваний розмір даних
     * @param filename Ім'я файлу налаштувань
     * @param subdir Ім'я підкаталогу налаштувань
     * @return true - якщо розмір очікуваних даних збігається з розміром прочитаних даних. false - інакше
     */
    static bool load(void* out_data_struct, size_t data_struct_size, const String& filename, const String& subdir = emptyString);

    /**
     * @brief Зберігає структуру налаштувань на карту пам'яті.
     *
     * @param data_struct Вказівник на пам'ять, звідки будуть скопійовані дані до файлу.
     * @param data_struct_size Розмір даних
     * @param filename Ім'я файлу налаштувань
     * @param subdir Ім'я підкаталогу налаштувань
     * @return true - якщо розмір даних збігається з розміром записаних даних. false - інакше
     */
    static bool save(const void* data_struct, size_t data_struct_size, const String& filename, const String& subdir = emptyString);
  };
}  // namespace pixeler
