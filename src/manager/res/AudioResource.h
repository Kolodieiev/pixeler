#pragma once
#pragma GCC optimize("O3")

#include "IResource.h"

namespace pixeler
{
  class AudioResource : public IResource
  {
  public:
    AudioResource(const char* name, void* data, uint32_t length) : IResource(name, data, TYPE_AUDIO), _LENGTH{length}
    {
    }

    virtual ~AudioResource() override {}

    /**
     * @brief Повертає довжину звукової доріжки.
     *
     * @return uint32_t
     */
    uint32_t getLen() const
    {
      return _LENGTH;
    }

  private:
    const uint32_t _LENGTH;
  };
}  // namespace pixeler
