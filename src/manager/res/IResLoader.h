/**
 * @file IResLoader.h
 * @brief Базовий інтерфейс для завантажувачів ресурсів різних форматів.
 */
#pragma once
#pragma GCC optimize("O3")
#include <stdint.h>

#include "IResource.h"
#include "defines.h"

namespace pixeler
{
  class IResLoader
  {
  public:
    IResLoader();
    virtual ~IResLoader() = 0;

    /**
     * @brief Завантажує ресурс із файлової системи в оперативну пам'ять.
     *
     * @param path Шлях до файлу ресурсу на диску.
     * @param name Необов'язкове символьне ім'я, яке буде присвоєно створеному ресурсу.
     * @return IResource* Вказівник на створений ресурс, якщо файл успішно розпізнано та завантажено.
     * @return nullptr У разі помилки читання або невідповідності формату.
     */
    virtual IResource* load(const char* path, const char* name = nullptr) = 0;

  protected:
  };
}  // namespace pixeler
