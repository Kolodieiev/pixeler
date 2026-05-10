#pragma once
#pragma GCC optimize("O3")

#include <stdint.h>

#include <unordered_map>

#include "TerrainManager.h"
#include "TileType.h"
#include "defines.h"
#include "manager/ResManager.h"

namespace pixeler
{
  class TerrainLoader
  {
  public:
    TerrainLoader();
    ~TerrainLoader();

    /**
     * @brief
     *
     * @param terr_csv_path Шлях до *.csv-файлу-шаблону поверхні сцени.
     * @param tile_type_path Шлях до конвертованого з набору плиток файлу, що містить інформацію про типи плиток. Див. TyleType.h
     * @param bmp_dir_path Шлях до каталогу, що містить 16 bit 565 bmp-зображення плиток.
     * @return true - Якщо всі дані поверхні було успішно прочитано та завантажено до пам'яті.
     * @return false - Інакше.
     */
    bool loadTerrain(const char* terr_csv_path, const char* tile_type_path, const char* bmp_dir_path);

    /**
     * @brief Повністю звільняє ресурси, які були завантажені раніше.
     * Автоматично викликається в деструкторі.
     * Можна викликати, якщо необхідно перезавантажити поверхню сцени.
     */
    void clearTerrainData();

    /**
     * @brief Збирає поверхню сцени на основі завантажених раніше даних.
     *
     * @param terrain Менеджер поверхні сцени.
     * @param tile_side_len Розмір однієї зі сторін плитки, з яких формується поточна поверхня.
     */
    void buildTerrain(TerrainManager& terrain, uint16_t tile_side_len);

  private:
    String loadFile(const char* path);
    bool loadTerrainTmpl(const char* terr_csv_path);
    bool loadTileTypeInfo(const char* tile_type_path);
    bool loadBmps(const char* bmp_dir_path);
    TileType textToTileType(const char* str, int start, int end);
    TileType getTileTypeByID(uint16_t tile_id);
    const uint16_t* getBmpDataByID(uint16_t tile_id);

  private:
    ResManager _res_manager;

    std::unordered_map<uint16_t, TileType> _tile_type_data;
    std::unordered_map<uint16_t, uint32_t> _terr_bmp_data;

    uint16_t* _terrain_tmpl{nullptr};
    uint16_t _terrain_width{0};
    uint16_t _terrain_height{0};
    bool _is_terrain_loaded{false};
  };

}  // namespace pixeler
