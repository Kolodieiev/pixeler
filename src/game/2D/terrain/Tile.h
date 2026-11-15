#pragma once
#include "TileType.h"
#include "defines.h"

namespace pixeler
{
  class Tile
  {
  public:
    Tile(TileType type, const uint16_t* img_data, bool is_transparent = false)
        : _img_data{img_data},
          _type{type},
          _is_transparent{is_transparent} {}

    const uint16_t* _img_data;
    const TileType _type;
    const bool _is_transparent;
  };
}  // namespace pixeler
