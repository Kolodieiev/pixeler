/**
 * @file AudioResource.h
 * @brief Клас звукового ресурсу, що зберігає буфер PCM-даних та його розмір.
 */
#pragma once
#pragma GCC optimize("O3")

#include "IResource.h"

namespace pixeler
{
  class AudioResource : public IResource
  {
  public:
    /**
     * @brief Створює екземпляр звукового ресурсу.
     *
     * @param name Символьне ім'я ресурсу (може бути nullptr або порожнім рядком).
     * @param data Вказівник на буфер із сирими аудіоданими (PCM).
     * @param length Розмір буфера аудіоданих у байтах.
     */
    AudioResource(const char* name, void* data, uint32_t length) : IResource(name, data, length, TYPE_AUDIO)
    {
    }

    virtual ~AudioResource() override {}

  private:
  };
}  // namespace pixeler
