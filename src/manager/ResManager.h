/**
 * @file ResManager.h
 * @brief Менеджер завантаження і керування bmp та wav ресурсами
 * @details Завантажує bmp-зображення та wav-файли в оперативну пам'ять пристрою.
 * Надає доступ до даних ресурсів по їх ідентифікатору або імені.
 * Звільняє пам'ять ресурсів по їх ідентифікатору або імені.
 * Не повинен реалізовуватися в користувацькому коді.
 * Доступний для використання через глобальний об'єкт "_res" в класі контексту.
 */

#pragma once
#pragma GCC optimize("O3")
#include <memory>
#include <unordered_map>

#include "../defines.h"
#include "res/IResLoader.h"
#include "res/IResource.h"

namespace pixeler
{
  class ResManager
  {
  public:
    /**
     * @brief Завантажує ресурс із файлової системи в оперативну пам'ять (RAM/PSRAM).
     * Тип завантажувача визначається автоматично за розширенням файлу (.bmp або .wav).
     *
     * @param path Шлях до файлу ресурсу на носії.
     * @param res_name Необов'язкове символьне ім'я ресурсу для подальшого пошуку за назвою.
     * @return uint32_t Унікальний числовий ідентифікатор завантаженого ресурсу (> 0),
     * або 0 у разі помилки завантаження.
     */
    uint32_t load(const char* path, const char* res_name = nullptr);

    /**
     * @brief Знаходить раніше завантажений ресурс за його символьним іменем.
     *
     * @param res_name Ім'я ресурсу, вказане під час завантаження.
     * @return const IResource* Вказівник на ресурс, або nullptr, якщо ресурс із таким іменем не знайдено.
     */
    const IResource* getResByName(const char* res_name) const;

    /**
     * @brief Знаходить раніше завантажений ресурс за його числовим ідентифікатором.
     *
     * @param res_id Ідентифікатор ресурсу, отриманий від методу load().
     * @return const IResource* Вказівник на ресурс, або nullptr, якщо ідентифікатор невідомий.
     */
    const IResource* getResByID(uint32_t res_id) const;

    /**
     * @brief Видаляє ресурс із кешу та звільняє виділену під нього пам'ять за символьним іменем.
     *
     * @param res_name Ім'я ресурсу, який потрібно видалити.
     */
    void deleteResByName(const char* res_name);

    /**
     * @brief Видаляє ресурс із кешу та звільняє виділену під нього пам'ять за числовим ідентифікатором.
     *
     * @param res_id Числовий ідентифікатор ресурсу, який потрібно видалити.
     */
    void deleteResByID(uint32_t res_id);

    /**
     * @brief Повністю очищує менеджер ресурсів, звільняючи пам'ять усіх завантажених ресурсів,
     * та скидає лічильник ідентифікаторів.
     */
    void freeRes();

    ResManager() {}
    ResManager(const ResManager&) = delete;
    ResManager& operator=(const ResManager&) = delete;
    ResManager(ResManager&&) = delete;
    ResManager& operator=(ResManager&&) = delete;

  private:
    std::unique_ptr<IResLoader> getLoaderByExt(const char* path);

  private:
    std::unordered_map<uint32_t, IResource*> _resources;  // <res_id, res_ptr>
    uint32_t _res_id_counter{0};
  };

  /**
   * @brief Глобальний об'єкт для завантаження ресурсів та керування ними.
   */
  extern ResManager _res;
}  // namespace pixeler
