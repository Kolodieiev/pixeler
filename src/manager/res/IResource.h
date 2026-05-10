#pragma once
#pragma GCC optimize("O3")

#include <stdint.h>

#include "defines.h"

namespace pixeler
{
  class IResource
  {
  public:
    enum ResType : uint8_t
    {
      TYPE_IMAGE = 0,
      TYPE_AUDIO
    };

    IResource(const String& name, void* data, ResType type);
    virtual ~IResource() = 0;

    /**
     * @brief Повертає ім'я ресурса.
     *
     * @return const String&
     */
    const char* getName() const;

    /**
     * @brief Перевіряє, чи має ресурс вказане ім'я.
     *
     * @param name Рядок, з яким буде порівнюватись ім'я ресурсу.
     * @return true - Якщо імена співпадають.
     * @return false - Інакше.
     */
    bool hasName(const char* name) const;

    /**
     * @brief Повертає дані ресурса.
     *
     * @return const void*
     */
    const void* getData() const;

    /**
     * @brief Повертає тип ресурса.
     *
     * @return ResType
     */
    ResType getType() const;

    IResource(const IResource&) = delete;
    IResource& operator=(const IResource&) = delete;
    IResource(IResource&& other) noexcept = delete;

  protected:
    char* _name{nullptr};
    void* _data{nullptr};
    const uint32_t _NAME_LEN;
    const ResType _RES_TYPE;

  private:
  };
}  // namespace pixeler
