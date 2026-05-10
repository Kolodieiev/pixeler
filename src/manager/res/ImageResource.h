#pragma once
#pragma GCC optimize("O3")

#include "IResource.h"

namespace pixeler
{
  class ImageResource : public IResource
  {
  public:
    ImageResource(const char* name, void* data, uint16_t width, uint16_t height) : IResource(name, data, TYPE_IMAGE), _WIDTH{width}, _HEIGHT{height}
    {
    }

    virtual ~ImageResource() override
    {
    }

    /**
     * @brief Повертає ширину зображення.
     *
     * @return uint16_t
     */
    uint16_t getWidth() const
    {
      return _WIDTH;
    }

    /**
     * @brief Повертає висоту зображення.
     *
     * @return uint16_t
     */
    uint16_t getHeight() const
    {
      return _HEIGHT;
    }

  private:
    const uint16_t _WIDTH{0};
    const uint16_t _HEIGHT{0};
  };
}  // namespace pixeler
