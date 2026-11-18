#pragma GCC optimize("O3")
#include "IGameObject2D.h"

#include <cmath>

#include "IGameScene2D.h"

namespace pixeler
{
  static constexpr float RAD_TO_DEG_F{57.2957795f};

  IGameObject2D::IGameObject2D(uint32_t id, uint16_t type_id, IGameScene2D& game_scene, SfxPlayer& audio)
      : _scene{game_scene},
        _sfx_player{audio},
        _OBJ_ID{id},
        _TYPE_ID{type_id}
  {
    _sprite_tmpl = _scene.getSpriteTemplate(_TYPE_ID);
  }

  IGameObject2D::~IGameObject2D()
  {
  }

  uint32_t IGameObject2D::getID() const
  {
    return _OBJ_ID;
  }

  uint16_t IGameObject2D::getTypeID() const
  {
    return _TYPE_ID;
  }

  void IGameObject2D::__onDraw()
  {
    const uint16_t* frame = nullptr;

    if (_sprite.has_animation)
    {
      if (!_sprite.animation || _sprite.animation->empty())
      {
        log_e("Не встановлено або порожній вектор анімації");
        esp_restart();
      }

      frame = (*_sprite.animation)[_sprite.anim_pos];

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
      if (!_sprite.image)
      {
        log_e("Не встановлено вказівник на зображення");
        esp_restart();
      }

      frame = _sprite.image;
    }
    else
    {
      return;
    }

    if (_sprite.angle == 0)
      _display.drawBitmapTransp(_x_local,
                                _y_local,
                                frame,
                                _geometry->width,
                                _geometry->height);
    else
      _display.drawBitmapRotated(_x_local,
                                 _y_local,
                                 frame,
                                 _geometry->width,
                                 _geometry->height,
                                 _geometry->x_pivot,
                                 _geometry->y_pivot,
                                 _sprite.angle);
  }

  void IGameObject2D::setPos(uint16_t x_pos, uint16_t y_pos)
  {
    _x_global = x_pos;
    _y_global = y_pos;
  }

  uint16_t IGameObject2D::getGlobalX() const
  {
    return _x_global;
  }

  uint16_t IGameObject2D::getGlobalY() const
  {
    return _y_global;
  }

  uint16_t IGameObject2D::calcAngleToPoint(uint16_t x, uint16_t y) const
  {
    const float dy = static_cast<float>(static_cast<int32_t>(y) - _y_global - _geometry->y_pivot);
    const float dx = static_cast<float>(static_cast<int32_t>(x) - _x_global - _geometry->x_pivot);

    int16_t azimut = static_cast<int16_t>(atan2f(dy, dx) * RAD_TO_DEG_F);
    if (azimut < 0)
      azimut += 360;

    return azimut + 90;
  }

  uint16_t IGameObject2D::calcDistToPoint(uint16_t x, uint16_t y) const
  {
    const int32_t dx = static_cast<int32_t>(_x_global) + _geometry->x_pivot - x;
    const int32_t dy = static_cast<int32_t>(_y_global) + _geometry->y_pivot - y;

    return static_cast<uint16_t>(sqrtf(static_cast<float>(dx * dx + dy * dy)));
  }

  void IGameObject2D::stepToPoint(uint16_t x_to, uint16_t y_to, uint16_t step_w)
  {
    if (step_w == 0 || (_x_global == x_to && _y_global == y_to))
      return;

    const int32_t dx = static_cast<int32_t>(x_to) - _x_global;
    const int32_t dy = static_cast<int32_t>(y_to) - _y_global;
    const int32_t abs_dx = std::abs(dx);
    const int32_t abs_dy = std::abs(dy);

    if (dx == 0)
    {
      if (abs_dy <= step_w)
        _y_global = y_to;
      else
        _y_global += (dy > 0) ? step_w : -static_cast<int32_t>(step_w);
      return;
    }

    if (dy == 0)
    {
      if (abs_dx <= step_w)
        _x_global = x_to;
      else
        _x_global += (dx > 0) ? step_w : -static_cast<int32_t>(step_w);
      return;
    }

    if (abs_dx >= abs_dy)
    {
      if (abs_dx <= step_w)
      {
        _x_global = x_to;
        _y_global = y_to;
      }
      else
      {
        _x_global += (dx > 0) ? step_w : -static_cast<int32_t>(step_w);
        _y_global += (static_cast<int32_t>(step_w) * dy) / abs_dx;
      }
    }
    else
    {
      if (abs_dy <= step_w)
      {
        _y_global = y_to;
        _x_global = x_to;
      }
      else
      {
        _y_global += (dy > 0) ? step_w : -static_cast<int32_t>(step_w);
        _x_global += (static_cast<int32_t>(step_w) * dx) / abs_dy;
      }
    }
  }

  void IGameObject2D::setAnimationVariant(uint8_t anim_variant_ID)
  {
    if (_sprite_tmpl->animation_variants.size() <= anim_variant_ID)
    {
      log_e("Відсутня анімація з ID [%u] для Type_ID [%u]", anim_variant_ID, _TYPE_ID);
      esp_restart();
    }

    _sprite.animation = &_sprite_tmpl->animation_variants[anim_variant_ID];
    _sprite.resetAnimation();
  }

  void IGameObject2D::setImgVariant(uint8_t img_variant_ID)
  {
    if (_sprite_tmpl->img_variants.size() <= img_variant_ID)
    {
      log_e("Відсутнє зображення спрайта з ID [%u] для Type_ID [%u]", img_variant_ID, _TYPE_ID);
      esp_restart();
    }

    _sprite.image = _sprite_tmpl->img_variants[img_variant_ID];
  }

  void IGameObject2D::setGeometryVariant(uint8_t geometry_variant_ID)
  {
    if (_sprite_tmpl->geometry_variants.size() <= geometry_variant_ID)
    {
      log_e("Відсутня геометрія спрайта з ID [%u] для Type_ID [%u]", geometry_variant_ID, _TYPE_ID);
      esp_restart();
    }

    _geometry = &_sprite_tmpl->geometry_variants[geometry_variant_ID];
  }

  bool IGameObject2D::hasIntersectWithPoint(uint16_t x, uint16_t y, bool rigid_only) const
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

  bool IGameObject2D::hasIntersectWithCircle(uint16_t x_mid, uint16_t y_mid, uint16_t radius, bool rigid_only) const
  {
    if (rigid_only && !_physics.is_rigid)
      return false;

    int32_t rect_x1;
    int32_t rect_y1;
    int32_t rect_x2;
    int32_t rect_y2;

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

    const int32_t cx = x_mid;
    const int32_t cy = y_mid;

    const uint32_t dx = (cx < rect_x1) ? (rect_x1 - cx) : (cx > rect_x2 ? (cx - rect_x2) : 0);
    if (dx > radius)
      return false;

    const uint32_t dy = (cy < rect_y1) ? (rect_y1 - cy) : (cy > rect_y2 ? (cy - rect_y2) : 0);
    if (dy > radius)
      return false;

    const uint32_t r = radius;
    return (dx * dx + dy * dy) <= (r * r);
  }

  bool IGameObject2D::hasIntersectWithRect(uint16_t x_start, uint16_t y_start, uint16_t rect_width, uint16_t rect_height, bool rigid_only) const
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
