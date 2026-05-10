// Перечислення містить типи прохідності плиток.
// Ці типи використовуються для обмеження переміщення ігрових об'єктів по ігровому рівні.
// Також за допомогою типу плитки можна регулювати швидкість переміщення об'єкта.
// TerrainLoader для визначення типу плитки очікує значення, вказане в коментарі до кожного типу.
// Типи плиток потрібно вказувати в наборі плиток в Tiled.
// Після створення набору плиток, необхідно розпарсити його *.tsx-файл скриптом tile_type_extractor.py,
// та передати шлях до згенерованого файлу в TerrainLoader.
// Вказувати тип плиток також можна вручну, як це продемонстровано в грі sokoban.
// Підказка: Tiled підтримує масове встановлення властивостей для плиток.

#pragma once
#include <stdint.h>

namespace pixeler
{
  enum TileType : uint16_t
  {
    TILE_TYPE_NONE = 0,             // none
    TILE_TYPE_WALL = 1 << 0,        // wall
    TILE_TYPE_GROUND = 1 << 1,      // ground
    TILE_TYPE_WATER = 1 << 2,       // water
    TILE_TYPE_AIR = 1 << 3,         // air
    TILE_TYPE_FIRE = 1 << 4,        // fire
    TILE_TYPE_SNOW = 1 << 5,        // snow
    TILE_TYPE_MUD = 1 << 6,         // mud
    TILE_TYPE_ROAD = 1 << 7,        // road
    TILE_TYPE_TREE = 1 << 8,        // tree
    TILE_TYPE_IMPASSABLE = 1 << 9,  // impass
    TILE_TYPE_ALL = 0xFFFF
  };

}  // namespace pixeler
