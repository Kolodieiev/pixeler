// Перечислення містить типи прохідності плиток.
// Ці типи використовуються для обмеження переміщення ігрових об'єктів по ігровому рівні.
// Також за допомогою типу плитки можна регулювати швидкість переміщення об'єкта.
// TerrainLoader2D для визначення типу плитки очікує значення, вказане в коментарі до кожного типу.
// Типи плиток потрібно вказувати в наборі плиток в Tiled.
// Після створення набору плиток, необхідно розпарсити його *.tsx-файл скриптом tile_type_extractor.py,
// та передати шлях до згенерованого файлу в TerrainLoader2D.
// Вказувати тип плиток також можна вручну, як це продемонстровано в грі sokoban.
// Підказка: Tiled підтримує масове встановлення властивостей для плиток.

#pragma once
#include <stdint.h>

namespace pixeler
{
  enum TileType : uint16_t
  {
    TILE_TYPE_NONE = 0,               // none
    TILE_TYPE_WALL = 1u << 0,         // wall
    TILE_TYPE_GROUND = 1u << 1,       // ground
    TILE_TYPE_WATER = 1u << 2,        // water
    TILE_TYPE_AIR = 1u << 3,          // air
    TILE_TYPE_FIRE = 1u << 4,         // fire
    TILE_TYPE_SNOW = 1u << 5,         // snow
    TILE_TYPE_MUD = 1u << 6,          // mud
    TILE_TYPE_ROAD = 1u << 7,         // road
    TILE_TYPE_TREE = 1u << 8,         // tree
    TILE_TYPE_ICE = 1u << 9,          // ice
    TILE_TYPE_LAVA = 1u << 10,        // lava
    TILE_TYPE_IMPASSABLE = 1u << 15,  // impass
    TILE_TYPE_ALL = 0xFFFF
  };

}  // namespace pixeler
