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
     * @brief Завантажує ресурс з диску.
     *
     * @param path Шлях до ресурса на диску.
     * @return IResource* - Якщо ресурс успішно розпізнано та завантажено.
     * @return nullptr - Інакше.
     */
    virtual IResource* load(const char* path, const char* name = nullptr) = 0;

  protected:
  };
}  // namespace pixeler
