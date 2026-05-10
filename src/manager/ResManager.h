/**
 * @file ResManager.h
 * @brief Менеджер завантаження і керування bmp та wav ресурсами
 * @details Завантажує bmp-зображення та wav-файли в оперативну пам'ять пристрою.
 * Надає доступ до даних ресурсів по їх ідентифікатору.
 * Звільняє пам'ять ресурсів по їх ідентифікатору.
 * Не повинен реалізовуватися в користувацьому коді.
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
    uint32_t load(const char* path, const char* res_name = nullptr);
    const IResource* getResByName(const char* res_name, uint32_t& out_id) const;
    const IResource* getResByID(uint32_t res_id) const;
    void deleteResByName(const char* res_name);
    void deleteResByID(uint32_t res_id);
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
   *
   */
  extern ResManager _res;
}  // namespace pixeler
