#pragma GCC optimize("O3")
#include "IGameObject.h"

#include <cmath>

#include "IGameScene.h"

namespace pixeler
{
  IGameObject::IGameObject(uint32_t id, uint16_t type_id, IGameScene& game_scene, SfxPlayer& audio)
      : _obj_ID{id},
        _type_ID{type_id},
        _scene{game_scene},
        _sfx_player{audio}
  {
    _sprite_tmpl = _scene.getSpriteTemplate(_type_ID);
  }

  IGameObject::~IGameObject()
  {
  }

  uint32_t IGameObject::getID() const
  {
    return _obj_ID;
  }

  uint16_t IGameObject::getTypeID() const
  {
    return _type_ID;
  }

  void IGameObject::__onDraw()
  {
    if (_sprite.has_animation)
    {
      if (!_sprite.animation) [[unlikely]]
      {
        log_e("Не встановлено вказівник на вектор анімації");
        esp_restart();
      }

      if (_sprite.angle == 0)
        _display.drawBitmapTransp(_x_local,
                                  _y_local,
                                  _sprite.animation->at(_sprite.anim_pos),
                                  _geometry->width,
                                  _geometry->height);
      else
        _display.drawBitmapRotated(_x_local,
                                   _y_local,
                                   _sprite.animation->at(_sprite.anim_pos),
                                   _geometry->width,
                                   _geometry->height,
                                   _geometry->x_pivot,
                                   _geometry->y_pivot,
                                   _sprite.angle);

      if (_sprite.frames_counter != _sprite.frames_between_anim)
      {
        ++_sprite.frames_counter;
      }
      else
      {
        _sprite.frames_counter = 0;
        if (_sprite.anim_pos < _sprite.animation->size() - 1)
          ++_sprite.anim_pos;
        else
          _sprite.anim_pos = 0;
      }
    }
    else if (_sprite.has_img)
    {
      if (!_sprite.image) [[unlikely]]
      {
        log_e("Не встановлено вказівник на зображення");
        esp_restart();
      }

      if (_sprite.angle == 0)
        _display.drawBitmapTransp(_x_local,
                                  _y_local,
                                  _sprite.image,
                                  _geometry->width,
                                  _geometry->height);
      else
        _display.drawBitmapRotated(_x_local,
                                   _y_local,
                                   _sprite.image,
                                   _geometry->width,
                                   _geometry->height,
                                   _geometry->x_pivot,
                                   _geometry->y_pivot,
                                   _sprite.angle);
    }
  }

  void IGameObject::setPos(uint16_t x_pos, uint16_t y_pos)
  {
    _x_global = x_pos;
    _y_global = y_pos;
  }

  uint16_t IGameObject::getGlobalX() const
  {
    return _x_global;
  }

  uint16_t IGameObject::getGlobalY() const
  {
    return _y_global;
  }

  uint16_t IGameObject::calcAngleToPoint(uint16_t x, uint16_t y)
  {
    int16_t azimut = atan2(y - _y_global - _geometry->y_pivot, x - _x_global - _geometry->x_pivot) * 180 / PI;
    if (azimut < 0)
      azimut += 360;
    return azimut + 90;
  }

  uint16_t IGameObject::calcDistToPoint(uint16_t x, uint16_t y)
  {
    uint16_t a = __builtin_abs(_x_global + _geometry->x_pivot - x);
    uint16_t b = __builtin_abs(_y_global + _geometry->y_pivot - y);

    return sqrt((a * a) + (b * b));
  }

  void IGameObject::stepToPoint(uint16_t x_to, uint16_t y_to, uint16_t step_w)
  {
    if (_x_global == x_to && _y_global == y_to)
      return;

    int32_t dx = static_cast<int32_t>(x_to) - _x_global;
    int32_t dy = static_cast<int32_t>(y_to) - _y_global;

    if (dx == 0)
    {  // Рух строго по вертикалі
      if (__builtin_abs(dy) <= step_w)
        _y_global = y_to;
      else
        _y_global += (dy > 0) ? step_w : -step_w;
      return;
    }

    if (dy == 0)
    {  // Рух строго по горизонталі
      if (__builtin_abs(dx) <= step_w)
        _x_global = x_to;
      else
        _x_global += (dx > 0) ? step_w : -step_w;
      return;
    }

    // Визначаємо, по якій осі відстань більша, щоб зробити її базовою
    if (__builtin_abs(dx) >= __builtin_abs(dy))
    {
      // Рух переважно по X
      int16_t x_prev = _x_global;
      if (__builtin_abs(dx) <= step_w)
      {
        _x_global = x_to;
        _y_global = y_to;
      }
      else
      {
        _x_global += (dx > 0) ? step_w : -step_w;
        // Множимо перед діленням для збереження точності
        _y_global += static_cast<int32_t>(_x_global - x_prev) * dy / dx;
      }
    }
    else
    {
      int16_t y_prev = _y_global;
      if (__builtin_abs(dy) <= step_w)
      {
        _y_global = y_to;
        _x_global = x_to;
      }
      else
      {
        _y_global += (dy > 0) ? step_w : -step_w;
        _x_global += static_cast<int32_t>(_y_global - y_prev) * dx / dy;
      }
    }
  }

  void IGameObject::setAnimationVariant(uint8_t anim_variant_ID)
  {
    if (_sprite_tmpl->animation_variants.size() <= anim_variant_ID) [[unlikely]]
    {
      log_e("Відсутня анімація з ID [%u] для Type_ID [%u]", anim_variant_ID, _type_ID);
      esp_restart();
    }

    _sprite.animation = &_sprite_tmpl->animation_variants[anim_variant_ID];
    _sprite.resetAnimation();
  }

  void IGameObject::setImgVariant(uint8_t img_variant_ID)
  {
    if (_sprite_tmpl->img_variants.size() <= img_variant_ID) [[unlikely]]
    {
      log_e("Відсутнє зображення спрайта з ID [%u] для Type_ID [%u]", img_variant_ID, _type_ID);
      esp_restart();
    }

    _sprite.image = _sprite_tmpl->img_variants[img_variant_ID];
  }

  void IGameObject::setGeometryVariant(uint8_t geometry_variant_ID)
  {
    if (_sprite_tmpl->geometry_variants.size() <= geometry_variant_ID) [[unlikely]]
    {
      log_e("Відсутня геометрія спрайта з ID [%u] для Type_ID [%u]", geometry_variant_ID, _type_ID);
      esp_restart();
    }

    _geometry = &_sprite_tmpl->geometry_variants[geometry_variant_ID];
  }

  bool
  IGameObject::hasIntersectWithPoint(uint16_t x, uint16_t y, bool rigid_only) const
  {
    if (rigid_only)
    {
      if (!_physics.is_rigid)
        return false;

      return (x >= _x_global + _geometry->rigid_offsets.left &&
              x <= _x_global + _geometry->width - _geometry->rigid_offsets.right - 1) &&
          (y >= _y_global + _geometry->rigid_offsets.top &&
           y <= _y_global + _geometry->height - _geometry->rigid_offsets.bottom - 1);
    }

    return (x >= _x_global &&
            x <= _x_global + _geometry->width - 1) &&
        (y >= _y_global &&
         y <= _y_global + _geometry->height - 1);
  }

  bool IGameObject::hasIntersectWithCircle(uint16_t x_mid, uint16_t y_mid, uint16_t radius, bool rigid_only) const
  {
    if (rigid_only && !_physics.is_rigid)
      return false;

    // 1. Визначаємо межі прямокутника (AABB)
    int16_t rect_x1, rect_y1, rect_x2, rect_y2;

    if (rigid_only)
    {
      rect_x1 = _x_global + _geometry->rigid_offsets.left;
      rect_y1 = _y_global + _geometry->rigid_offsets.top;
      rect_x2 = _x_global + _geometry->width - _geometry->rigid_offsets.right;
      rect_y2 = _y_global + _geometry->height - _geometry->rigid_offsets.bottom;
    }
    else
    {
      rect_x1 = _x_global;
      rect_y1 = _y_global;
      rect_x2 = _x_global + _geometry->width;
      rect_y2 = _y_global + _geometry->height;
    }

    // 2. Знаходимо найближчу точку на прямокутнику до центру кола
    int16_t closest_x = (x_mid < rect_x1) ? rect_x1 : (x_mid > rect_x2 ? rect_x2 : x_mid);
    int16_t closest_y = (y_mid < rect_y1) ? rect_y1 : (y_mid > rect_y2 ? rect_y2 : y_mid);

    // 3. Рахуємо відстань від найближчої точки до центру кола
    int32_t dx = static_cast<int32_t>(x_mid) - closest_x;
    int32_t dy = static_cast<int32_t>(y_mid) - closest_y;

    // 4. Перевірка: чи відстань менша за радіус (використовуємо квадрат відстані)
    return (dx * dx + dy * dy) <= (static_cast<int32_t>(radius) * radius);
  }

  bool IGameObject::hasIntersectWithRect(uint16_t x_start, uint16_t y_start, uint16_t rect_width, uint16_t rect_height, bool rigid_only) const
  {
    if (rigid_only)
    {
      if (!_physics.is_rigid)
        return false;

      if (_x_global + _geometry->rigid_offsets.left > x_start + rect_width ||
          x_start > _x_global + _geometry->width - _geometry->rigid_offsets.right - 1 ||
          _y_global + _geometry->rigid_offsets.top > y_start + rect_height ||
          y_start > _y_global + _geometry->height - _geometry->rigid_offsets.bottom - 1)
        return false;
    }
    else
    {
      if (_x_global > x_start + rect_width ||
          x_start > _x_global + _geometry->width - 1 ||
          _y_global > y_start + rect_height ||
          y_start > _y_global + _geometry->height - 1)
        return false;
    }

    return true;
  }

}  // namespace pixeler
