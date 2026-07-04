#pragma once
#include "../../defines.h"
#include "TileType.h"

namespace pixeler
{
  class Tile
  {
  public:
    Tile(TileType type, const uint16_t* img_data, bool has_transparency = false) : _img_data{img_data}, _type{type}, _has_transparency{has_transparency} {}

    const uint16_t* _img_data;
    const TileType _type;
    const bool _has_transparency;
  };
}  // namespace pixeler
