#pragma GCC optimize("O3")
#include "TerrainManager2D.h"

// #include "../IGameScene2D.h"

namespace pixeler
{
  TerrainManager2D::~TerrainManager2D()
  {
    freeMem();
    freeTilesDescriptionData();
  }

  void TerrainManager2D::setBackColorFilling(bool state)
  {
    _has_back_color = state;
  }

  void TerrainManager2D::freeMem()
  {
    if (!_terrain)
      return;

    for (uint16_t i = 0; i < _tile_y_num; ++i)
      free(_terrain[i]);

    free(_terrain);
    _terrain = nullptr;
  }

  void TerrainManager2D::freeTilesDescriptionData()
  {
    for (auto it = _tile_descr.begin(), last_it = _tile_descr.end(); it != last_it; ++it)
      delete it->second;

    _tile_descr.clear();
  }

  void TerrainManager2D::setCameraPos(uint16_t x, uint16_t y)
  {
    // X
    if (x < HALF_VIEW_W)
      _view_x = 0;
    else if (x < _width - HALF_VIEW_W)
      _view_x = x - HALF_VIEW_W;
    else
      _view_x = _width - VIEW_W;

    // Y
    if (y < HALF_VIEW_H)
      _view_y = 0;
    else if (y < _height - HALF_VIEW_H)
      _view_y = y - HALF_VIEW_H;
    else
      _view_y = _height - VIEW_H;
  }

  uint16_t TerrainManager2D::coordToTilePos(uint16_t coord) const
  {
    float temp_float_div = (float)coord / _tile_side_len;
    uint16_t rounded_div = round(temp_float_div);

    if (rounded_div > temp_float_div)
      --rounded_div;

    return rounded_div;
  }

  void TerrainManager2D::onDraw()
  {
    if (_has_back_color)
    {
#if CONFIG_IDF_TARGET_ESP32P4
      if (VIEW_W * VIEW_H > PPA_FILL_SIZE_TRIGG)
      {
        const bool old_ppa_state = _display.isPPAEnabled();
        _display.setPPA(true);
        _display.fillRect(0, 0, VIEW_W, VIEW_H, _back_color);
        _display.setPPA(old_ppa_state);
      }
      else
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
      {
        _display.fillRect(0, 0, VIEW_W, VIEW_H, _back_color);
      }
    }

    renderParallax();

    if (_terrain)
    {
      uint16_t first_tile_y_pos = coordToTilePos(_view_y);
      uint16_t first_tile_x_pos = coordToTilePos(_view_x);

      uint16_t last_tile_x_pos = coordToTilePos(_view_x + VIEW_W);
      uint16_t last_tile_y_pos = coordToTilePos(_view_y + VIEW_H);

      ++last_tile_y_pos;
      ++last_tile_x_pos;

      if (last_tile_y_pos > _tile_y_num)
        last_tile_y_pos = _tile_y_num;

      if (last_tile_x_pos > _tile_x_num)
        last_tile_x_pos = _tile_x_num;

      const int32_t x_draw_pos = (_view_x / _tile_side_len) * _tile_side_len - _view_x;
      int32_t y_draw_pos = (_view_y / _tile_side_len) * _tile_side_len - _view_y;

      int32_t temp_x_draw_pos = x_draw_pos;

#if CONFIG_IDF_TARGET_ESP32P4
      const bool old_ppa_state = _display.isPPAEnabled();
      _display.setPPA(_tile_side_len * _tile_side_len > PPA_IMG_SIZE_TRIGG);
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

      for (uint16_t h = first_tile_y_pos; h < last_tile_y_pos; ++h)
      {
        for (uint16_t w = first_tile_x_pos; w < last_tile_x_pos; ++w)
        {
          if (_terrain[h][w]->_type != TILE_TYPE_NONE)
          {
            if (!_terrain[h][w]->_has_transparency)
              _display.drawBitmap(temp_x_draw_pos, y_draw_pos, _terrain[h][w]->_img_data, _tile_side_len, _tile_side_len);
            else
              _display.drawBitmapTransp(temp_x_draw_pos, y_draw_pos, _terrain[h][w]->_img_data, _tile_side_len, _tile_side_len);
          }

          temp_x_draw_pos += _tile_side_len;
        }

        temp_x_draw_pos = x_draw_pos;
        y_draw_pos += _tile_side_len;
      }

#if CONFIG_IDF_TARGET_ESP32P4
      _display.setPPA(old_ppa_state);
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
    }
  }

  void TerrainManager2D::renderParallax()
  {
    if (_parallax_layers.empty())
      return;

#if CONFIG_IDF_TARGET_ESP32P4
    const bool old_ppa_state = _display.isPPAEnabled();
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

    for (auto& layer : _parallax_layers)
    {
      // Оновлюємо авторух по X (робимо це до перевірки по Y, щоб хмари рухались поза кадром)
      if (layer.speed_x != 0.0f)
      {
        layer.accum_x += layer.speed_x;
        if (layer.accum_x >= layer.width)
          layer.accum_x -= layer.width;
        else if (layer.accum_x <= -static_cast<float>(layer.width))
          layer.accum_x += layer.width;
      }

      // Перевіряємо видимість по Y
      int32_t draw_y = layer.offset_y - static_cast<int32_t>(_view_y * layer.scroll_ratio);
      if (draw_y + layer.height <= 0 || draw_y >= VIEW_H)
        continue;  // Шар повністю вище або нижче екрана

#if CONFIG_IDF_TARGET_ESP32P4
      // Обчислюємо один раз кількість видимих пікселів по висоті
      const int32_t vis_y1 = draw_y < 0 ? 0 : draw_y;
      const int32_t vis_y2 = draw_y + layer.height > VIEW_H ? VIEW_H : draw_y + layer.height;
      const uint32_t vis_h = static_cast<uint32_t>(vis_y2 - vis_y1);
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

      // Обчислюємо координати по X
      int32_t raw_x = layer.offset_x - static_cast<int32_t>(_view_x * layer.scroll_ratio) + static_cast<int32_t>(layer.accum_x);

      int32_t start_x = raw_x;
      int32_t end_x = raw_x + 1;

      if (layer.repeat_x)
      {
        start_x = raw_x % static_cast<int32_t>(layer.width);
        if (start_x > 0)
          start_x -= layer.width;
        end_x = VIEW_W;
      }

      // Відмальовуємо видимі частини зображення
      for (int32_t draw_x = start_x; draw_x < end_x; draw_x += layer.width)
      {
        if (draw_x + layer.width <= 0 || draw_x >= VIEW_W)
          continue;

#if CONFIG_IDF_TARGET_ESP32P4
        int32_t vis_x1 = draw_x < 0 ? 0 : draw_x;
        int32_t vis_x2 = draw_x + layer.width > VIEW_W ? VIEW_W : draw_x + layer.width;
        _display.setPPA((static_cast<uint32_t>(vis_x2 - vis_x1) * vis_h) > PPA_IMG_SIZE_TRIGG);
#endif  // #if CONFIG_IDF_TARGET_ESP32P4

        _display.drawBitmap(draw_x, draw_y, layer.img_data, layer.width, layer.height);
      }
    }

#if CONFIG_IDF_TARGET_ESP32P4
    _display.setPPA(old_ppa_state);
#endif  // #if CONFIG_IDF_TARGET_ESP32P4
  }

  void TerrainManager2D::build(uint16_t tiles_w_num, uint16_t tiles_h_num, uint16_t tile_side_len, const uint16_t* tiles_pos_tmpl)
  {
    if (tiles_w_num == 0 || tiles_h_num == 0 || tile_side_len == 0 || !tiles_pos_tmpl)
    {
      log_e("Некоректні параметри");
      esp_restart();
    }

    if (_tile_descr.empty())
    {
      log_e("Відсутній опис плиток ігрового рівня");
      esp_restart();
    }

    if (!psramInit())
    {
      log_e("Відсутня PSRAM");
      esp_restart();
    }

    _width = tiles_w_num * tile_side_len;
    _height = tiles_h_num * tile_side_len;

    if (_width < VIEW_W || _height < VIEW_H)
    {
      log_e("Розмір ігрового рівня не може бути меншим за ViewPort");
      esp_restart();
    }

    //
    _tile_side_len = tile_side_len;
    //
    _tile_x_num = tiles_w_num;
    _tile_y_num = tiles_h_num;

    freeMem();  // Звільнити ресурси, якщо було викликано не перший раз

    _terrain = static_cast<Tile***>(ps_malloc(sizeof(Tile**) * _tile_y_num));

    if (!_terrain)
    {
      log_e("Помилка виділення пам'яті");
      esp_restart();
    }

    for (uint16_t i{0}; i < _tile_y_num; ++i)
    {
      _terrain[i] = static_cast<Tile**>(ps_malloc(sizeof(Tile*) * _tile_x_num));

      if (!_terrain[i])
      {
        log_e("Помилка виділення пам'яті");
        esp_restart();
      }
    }

    uint32_t build_pos{0};
    for (uint16_t i{0}; i < _tile_y_num; ++i)
    {
      for (uint16_t j{0}; j < _tile_x_num; ++j)
      {
        uint16_t sprite_id = tiles_pos_tmpl[build_pos];
        _terrain[i][j] = _tile_descr.at(sprite_id);
        ++build_pos;
      }
    }
  }

  bool TerrainManager2D::canPass(uint16_t x_from,
                                 uint16_t y_from,
                                 uint16_t x_to,
                                 uint16_t y_to,
                                 const PhysicsState& physics,
                                 const SpriteGeometry& geometry) const
  {
    if (x_to > _width || y_to > _height || !_terrain)
      return false;

    if (!physics.is_rigid)
      return true;

    if (y_to < y_from)  // UP
    {
      uint16_t tile_y_pos = coordToTilePos(y_to + geometry.rigid_offsets.top);  // y верхнього краю тіла
      if (tile_y_pos >= _tile_y_num)
        return false;

      uint16_t tile_x_main_pos = coordToTilePos(x_to + geometry.rigid_offsets.left);  // х лівого краю тіла
      if (tile_x_main_pos >= _tile_x_num)
        return false;

      if (_terrain[tile_y_pos][tile_x_main_pos]->_type & physics.pass_ability_mask)  // чи проходить лівий кут тіла
      {
        uint16_t tile_x_side_pos = coordToTilePos(x_to + geometry.width - geometry.rigid_offsets.right - 1);  // х правого краю
        if (tile_x_side_pos >= _tile_x_num)
          return false;

        return _terrain[tile_y_pos][tile_x_side_pos]->_type & physics.pass_ability_mask;  // якщо правий проходить, то тіло проходить
      }
    }
    else if (y_to > y_from)  // DOWN
    {
      uint16_t tile_y_pos = coordToTilePos(y_to + geometry.height - geometry.rigid_offsets.bottom - 1);  // y куди повинна стати нижня сторона тіла
      if (tile_y_pos >= _tile_y_num)
        return false;

      uint16_t tile_x_main_pos = coordToTilePos(x_to + geometry.rigid_offsets.left);  // х лівого краю
      if (tile_x_main_pos >= _tile_x_num)
        return false;

      if (_terrain[tile_y_pos][tile_x_main_pos]->_type & physics.pass_ability_mask)
      {
        uint16_t tile_x_side_pos = coordToTilePos(x_to + geometry.width - geometry.rigid_offsets.right - 1);  // х правого краю
        if (tile_x_side_pos >= _tile_x_num)
          return false;

        return _terrain[tile_y_pos][tile_x_side_pos]->_type & physics.pass_ability_mask;
      }
    }
    else if (x_to < x_from)  // LEFT
    {
      uint16_t tile_x_pos = coordToTilePos(x_to + geometry.rigid_offsets.left);  // х куди стане ліва сторона тіла
      if (tile_x_pos >= _tile_x_num)
        return false;

      uint16_t tile_y_main_pos = coordToTilePos(y_to + geometry.rigid_offsets.top);  // у верхнього краю
      if (tile_y_main_pos >= _tile_y_num)
        return false;

      if (_terrain[tile_y_main_pos][tile_x_pos]->_type & physics.pass_ability_mask)
      {
        uint16_t tile_y_side_pos = coordToTilePos(y_to + geometry.height - geometry.rigid_offsets.bottom - 1);  // у нижнього краю
        if (tile_y_side_pos >= _tile_y_num)
          return false;

        return _terrain[tile_y_side_pos][tile_x_pos]->_type & physics.pass_ability_mask;
      }
    }
    else if (x_to > x_from)  // RIGHT
    {
      uint16_t tile_x_pos = coordToTilePos(x_to + geometry.width - geometry.rigid_offsets.right - 1);  // права сторона тіла
      if (tile_x_pos >= _tile_x_num)
        return false;

      uint16_t tile_y_main_pos = coordToTilePos(y_to + geometry.rigid_offsets.top);  // у верхнього краю
      if (tile_y_main_pos >= _tile_y_num)
        return false;

      if (_terrain[tile_y_main_pos][tile_x_pos]->_type & physics.pass_ability_mask)
      {
        uint16_t tile_y_side_pos = coordToTilePos(y_to + geometry.height - geometry.rigid_offsets.bottom - 1);  // у нижнього краю
        if (tile_y_side_pos >= _tile_y_num)
          return false;

        return _terrain[tile_y_side_pos][tile_x_pos]->_type & physics.pass_ability_mask;
      }
    }

    return false;
  }

  TileType TerrainManager2D::getTileType(uint16_t x, uint16_t y) const
  {
    if (x > _width || y > _height || !_terrain)
      return TILE_TYPE_NONE;

    uint16_t tile_y_pos = coordToTilePos(y);
    uint16_t tile_x_pos = coordToTilePos(x);
    return _terrain[tile_y_pos][tile_x_pos]->_type;
  }

  bool TerrainManager2D::isInView(uint16_t x_pos, uint16_t y_pos, uint16_t sprite_w, uint16_t sprite_h) const
  {
    if (x_pos + sprite_w < _view_x || y_pos + sprite_h < _view_y || x_pos > _view_x + VIEW_W || y_pos > _view_y + VIEW_H)
      return false;

    return true;
  }

  void TerrainManager2D::addParallaxLayer(const ParallaxLayer& layer)
  {
    _parallax_layers.emplace_back(layer);
  }

  void TerrainManager2D::clearParallaxLayers()
  {
    _parallax_layers.clear();
  }

  void TerrainManager2D::setBackColor(uint16_t color)
  {
    _back_color = color;
  }

  uint16_t TerrainManager2D::getBackColor() const
  {
    return _back_color;
  }

  void TerrainManager2D::addTileInfo(uint16_t sprite_id, TileType type, const uint16_t* sprite_src)
  {
    try
    {
      Tile* tile = new Tile(type, sprite_src);
      _tile_descr.insert(std::pair<uint16_t, Tile*>(sprite_id, tile));
    }
    catch (const std::bad_alloc& e)
    {
      log_e("%s", e.what());
      esp_restart();
    }
  }

  void TerrainManager2D::clearTilesInfos()
  {
    freeTilesDescriptionData();
  }
}  // namespace pixeler
