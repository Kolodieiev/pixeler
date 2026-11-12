/**
 * @file IResource.h
 * @brief Базовий абстрактний клас для всіх типів завантажуваних ресурсів.
 */
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

    /**
     * @brief Конструктор базового ресурсу.
     *
     * @param name Ім'я ресурсу.
     * @param data Вказівник на виділений у купі буфер із сирими даними ресурсу.
     * @param data_size Розмір даних ресурсу.
     * @param type Тип ресурсу (TYPE_IMAGE або TYPE_AUDIO).
     */
    IResource(const String& name, void* data, size_t data_size, ResType type);
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
     * @param name Рядок, з яким буде порівнюватись ім'я ресурсу
     * @return true - якщо імена збігаються. false - інакше
     */
    bool hasName(const char* name) const;

    /**
     * @brief Повертає дані ресурса.
     *
     * @return const void*
     */
    const void* getData() const;

    /**
     * @brief Повертає розмір даних ресурса.
     *
     * @return const size_t
     */
    const size_t getDataSize() const;

    /**
     * @brief Повертає тип ресурса.
     *
     * @return ResType
     */
    ResType getType() const;

    IResource(const IResource&) = delete;
    IResource& operator=(const IResource&) = delete;
    IResource(IResource&& other) noexcept = delete;

  private:
    char* _name{nullptr};
    void* _data{nullptr};
    const size_t _DATA_SIZE{0};
    const uint32_t _NAME_LEN;
    const ResType _RES_TYPE;
  };
}  // namespace pixeler
