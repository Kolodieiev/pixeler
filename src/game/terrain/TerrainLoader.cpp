#pragma GCC optimize("O3")
#include "TerrainLoader.h"

#include "manager/FileManager.h"

namespace pixeler
{
  TileType TerrainLoader::textToTileType(const char* str, int start, int end)
  {
    int len = end - start;

    if (len == 4 && strncmp(&str[start], "wall", 4) == 0)
      return TILE_TYPE_WALL;
    if (len == 5 && strncmp(&str[start], "water", 5) == 0)
      return TILE_TYPE_WATER;
    if (len == 6 && strncmp(&str[start], "ground", 6) == 0)
      return TILE_TYPE_GROUND;
    if (len == 3 && strncmp(&str[start], "air", 3) == 0)
      return TILE_TYPE_AIR;
    if (len == 4 && strncmp(&str[start], "fire", 4) == 0)
      return TILE_TYPE_FIRE;
    if (len == 4 && strncmp(&str[start], "snow", 4) == 0)
      return TILE_TYPE_SNOW;
    if (len == 3 && strncmp(&str[start], "mud", 3) == 0)
      return TILE_TYPE_MUD;
    if (len == 4 && strncmp(&str[start], "road", 4) == 0)
      return TILE_TYPE_ROAD;
    if (len == 4 && strncmp(&str[start], "tree", 4) == 0)
      return TILE_TYPE_TREE;
    if (len == 6 && strncmp(&str[start], "impass", 6) == 0)
      return TILE_TYPE_IMPASSABLE;

    return TILE_TYPE_NONE;
  }

  TerrainLoader::TerrainLoader()
  {
  }

  TerrainLoader::~TerrainLoader()
  {
    clearTerrainData();
  }

  TileType TerrainLoader::getTileTypeByID(uint16_t tile_id)
  {
    const auto tile_it = _tile_type_data.find(tile_id);

    if (tile_it == _tile_type_data.end())
    {
      log_e("Відсутній тип для плитки з ID: %u", tile_id);
      return TILE_TYPE_NONE;
    }

    return tile_it->second;
  }

  const uint16_t* TerrainLoader::getBmpDataByID(uint16_t tile_id)
  {
    const auto bmp_data_it = _terr_bmp_data.find(tile_id);

    if (bmp_data_it == _terr_bmp_data.end())
    {
      log_e("Відсутнє bmp для плитки з ID: %u", tile_id);
      return nullptr;
    }

    uint32_t res_id = bmp_data_it->second;
    return static_cast<const uint16_t*>(_res_manager.getResByID(res_id)->getData());
  }

  void TerrainLoader::clearTerrainData()
  {
    free(_terrain_tmpl);
    _terrain_tmpl = nullptr;

    _tile_type_data.clear();

    for (auto it_b = _terr_bmp_data.begin(), it_e = _terr_bmp_data.end(); it_b != it_e; ++it_b)
      _res_manager.deleteResByID(it_b->second);

    _terr_bmp_data.clear();

    _terrain_width = 0;
    _terrain_height = 0;

    _is_terrain_loaded = false;
  }

  void TerrainLoader::buildTerrain(TerrainManager& terrain, uint16_t tile_side_len)
  {
    if (!_is_terrain_loaded)
    {
      log_e("Дані мапи не було завантажено");
      return;
    }

    uint16_t terr_size = _terrain_width * _terrain_height;

    for (uint16_t i = 0; i < terr_size; ++i)
    {
      const uint16_t tile_id = _terrain_tmpl[i];

      const uint16_t* bmp_data = getBmpDataByID(tile_id);
      if (bmp_data)
        terrain.addTileDesc(tile_id, getTileTypeByID(tile_id), bmp_data);
    }

    terrain.build(_terrain_width, _terrain_height, tile_side_len, _terrain_tmpl);  // Побудувати мапу на основі даних шаблону
  }

  bool TerrainLoader::loadTerrain(const char* terr_csv_path, const char* tile_type_path, const char* bmp_dir_path)
  {
    clearTerrainData();

    if (!terr_csv_path || !tile_type_path || !bmp_dir_path)
    {
      log_e("Аргументи loadTerrain не повинні бути null");
      return false;
    }

    // Якщо перше не виконалось, пропускаємо інші завантаження
    if (loadTerrainTmpl(terr_csv_path) && loadTileTypeInfo(tile_type_path) && loadBmps(bmp_dir_path))
      _is_terrain_loaded = true;

    return _is_terrain_loaded;
  }

  String TerrainLoader::loadFile(const char* path)
  {
    if (!psramInit())
    {
      log_e("TerrainLoader може коректно працювати тільки за наявності PSRAM");
      return emptyString;
    }

    size_t file_size = _fs.getFileSize(path);
    if (file_size == 0)
    {
      log_e("Файл порожній: %s", path);
      return emptyString;
    }

    char* file_buf = static_cast<char*>(ps_malloc(file_size + 1));
    if (!file_buf)
    {
      log_e("Помилка виділення пам'яті для файлу: %s", path);
      return emptyString;
    }

    size_t byte_read = _fs.readFile(path, file_buf, file_size);

    if (byte_read == 0 || byte_read != file_size)
    {
      free(file_buf);
      return emptyString;
    }

    file_buf[file_size] = '\0';
    String ret_str = file_buf;

    free(file_buf);

    return ret_str;
  }

  bool TerrainLoader::loadTerrainTmpl(const char* terr_csv_path)
  {
    String content = loadFile(terr_csv_path);
    if (content.isEmpty())
    {
      log_e("Помилка читання шаблону поверхні");
      return false;
    }

    bool is_first_row = true;
    int len = content.length();

    for (int i = 0; i < len; i++)
    {
      if (content[i] == '\n')
      {
        ++_terrain_width;
        is_first_row = false;
      }
      else if (is_first_row && content[i] == ',')
      {
        ++_terrain_height;
      }
    }
    ++_terrain_height;

    _terrain_tmpl = static_cast<uint16_t*>(malloc(_terrain_width * _terrain_height * sizeof(uint16_t)));
    if (!_terrain_tmpl)
    {
      log_e("Помилка виділення пам'яті для шаблону поверхні");
      return false;
    }

    int row = 0, col = 0;
    uint16_t num = 0;

    for (int i = 0; i < len; ++i)
    {
      char ch = content[i];

      if (ch >= '0' && ch <= '9')
      {
        num = num * 10 + (ch - '0');
      }
      else if (ch == ',' || ch == '\n')
      {
        _terrain_tmpl[row * _terrain_height + col] = num;
        num = 0;
        ++col;

        if (ch == '\n')
        {
          ++row;
          col = 0;
        }
      }
      else if (ch == ' ' || ch == '\t' || ch == '\r')
      {
        continue;
      }
      else
      {
        log_e("Файл поверхні пошкоджений на позиції %d: неочікуваний символ '%c' (0x%02X)",
              i,
              (ch >= 32 && ch < 127) ? ch : '?',
              (unsigned char)ch);
        return false;
      }
    }

    return true;
  }

  bool TerrainLoader::loadTileTypeInfo(const char* tile_type_path)
  {
    String content = loadFile(tile_type_path);
    if (content.isEmpty())
    {
      log_e("Помилка читання типів плиток");
      return false;
    }

    _tile_type_data.reserve(20);
    int len = content.length();

    uint16_t id = 0;
    int value_start = 0;
    bool reading_id = true;

    for (int i = 0; i <= len; ++i)
    {
      char ch = (i < len) ? content[i] : '\n';

      if (reading_id)
      {
        if (ch >= '0' && ch <= '9')
        {
          id = id * 10 + (ch - '0');
        }
        else if (ch == '=')
        {
          reading_id = false;
          value_start = i + 1;
        }
        else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
          continue;
        }
        else
        {
          log_e("Файл типів плиток пошкоджений на позиції %d: неочікуваний символ '%c' (0x%02X)",
                i,
                (ch >= 32 && ch < 127) ? ch : '?',
                (unsigned char)ch);
          return false;
        }
      }
      else
      {
        if (ch == '\n' || ch == '\r' || i == len)
        {
          TileType type = textToTileType(content.c_str(), value_start, i);
          auto [iter, inserted] = _tile_type_data.try_emplace(id, type);

          if (!inserted)
          {
            log_e("Виявлено дублікат ID плитки: %u", id);
            return false;
          }

          id = 0;
          reading_id = true;
        }
        else if (ch == '\r')
        {
          continue;
        }
      }
    }

    return true;
  }

  bool TerrainLoader::loadBmps(const char* bmp_dir_path)
  {
    uint16_t terrain_size = _terrain_width * _terrain_height;

    _terr_bmp_data.reserve(terrain_size);

    for (uint16_t i = 0; i < terrain_size; ++i)
    {
      uint16_t tile_id = _terrain_tmpl[i];
      const auto it = _terr_bmp_data.find(tile_id);

      if (it != _terr_bmp_data.end())
        continue;

      String cur_file_name = bmp_dir_path;
      cur_file_name += tile_id;
      cur_file_name += ".bmp";

      uint32_t res_id = _res_manager.load(cur_file_name.c_str());

      if (res_id == 0)
        return false;

      _terr_bmp_data.emplace(tile_id, res_id);
    }

    return true;
  }

}  // namespace pixeler