#pragma once
#include "../../defines.h"
#include "TileType.h"

namespace pixeler
{
  class Tile
  {
  public:
    Tile(TileType type, const uint16_t* img_ptr, bool has_transparency = false) : _img_ptr{img_ptr}, _type{type}, _has_transparency{has_transparency} {}

    const uint16_t* _img_ptr;
    const TileType _type;
    const bool _has_transparency;
  };
}  // namespace pixeler