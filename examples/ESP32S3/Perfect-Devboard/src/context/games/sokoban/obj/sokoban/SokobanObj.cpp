#include "SokobanObj.h"

#include "../TriggerID.h"
#include "../TypeID.h"
#include "./res/sprite/sprite_sokoban.h"
#include "game/IGameScene.h"

namespace sokoban
{
  SokobanObj::SokobanObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IGameObject(id, TYPE_HERO, game_scene, audio)
  {
    _layer = 1;                         // Об'єкт повинен бути вище об'єктів точок
    _sprite.img_data = SPRITE_SOKOBAN;  // Встановити зображення спрайта
    _sprite.has_img = true;             // Указати, що об'єкт може малювати свій спрайт
    _sprite.width = 32;                 // Ширина спрайта
    _sprite.height = 32;                // Висота спрайта

    _sprite.pass_abillity_mask |= TILE_TYPE_GROUND;  // Маска типу прохідності ігрового об'єкта.
                                                     // Дозволяє обмежувати пересування об'єкта по певних видах плиток ігрової мапи
  }

  void SokobanObj::__update()
  {
    bool done{true};

    for (uint8_t i{0}; i < _boxes.size(); ++i)
    {
      if (!_boxes[i]->isOk())
      {
        done = false;
        break;
      }
    }

    if (done)
    {
      _trigger_ID = TriggerID::TRIGGER_NEXT_SCENE;
      _is_triggered = true;
    }
  }

  void SokobanObj::serialize(DataStream& ds) const
  {
  }

  void SokobanObj::deserialize(DataStream& ds)
  {
  }

  size_t SokobanObj::getDataSize() const
  {
    return 0;
  }

  void SokobanObj::__onDraw()
  {
    // Необовязковий метод
    // Якщо перевизначено, тут можна відрисувати все, що НЕ стосується спрайта об'єкта.
    // Наприклад, полоску XP над ним.

    IGameObject::__onDraw();  // Необхідно обов'язково викликати бітьківський метод для відрисовки спрайту чи анімації.
  }

  void SokobanObj::move(MovingDirection direction)
  {
    if (direction == DIRECTION_NONE)
      return;

    if (direction == DIRECTION_UP)
      stepTo(_x_global, _y_global - PIX_PER_STEP, _x_global, _y_global - PIX_PER_STEP * 2);
    else if (direction == DIRECTION_DOWN)
      stepTo(_x_global, _y_global + PIX_PER_STEP, _x_global, _y_global + PIX_PER_STEP * 2);
    else if (direction == DIRECTION_LEFT)
      stepTo(_x_global - PIX_PER_STEP, _y_global, _x_global - PIX_PER_STEP * 2, _y_global);
    else if (direction == DIRECTION_RIGHT)
      stepTo(_x_global + PIX_PER_STEP, _y_global, _x_global + PIX_PER_STEP * 2, _y_global);
  }

  void SokobanObj::addBoxPtr(BoxObj* box_ptr)
  {
    _boxes.push_back(box_ptr);
  }

  void SokobanObj::stepTo(uint16_t x, uint16_t y, uint16_t box_x_step, uint16_t box_y_step)
  {
    const std::array types = {(uint16_t)TYPE_BOX};

    // Шукаємо ящик на плитці в напрямку руху
    std::vector<IGameObject*> objs = _scene.getObjByTypeAt(types, x, y, this);

    if (!objs.empty())
    {
      BoxObj* box = static_cast<BoxObj*>(objs[0]);
      if (box->moveTo(box_x_step, box_y_step))  // намагаємося посунути ящик
      {
        _x_global = x;  // Якщо успішно, рухаємось за ящиком
        _y_global = y;
      }
      return;
    }

    // Якщо ящик не було знайдено, перевіряємо чи може комірник пройти
    if (_scene.canPass(*this, x, y))
    {
      _x_global = x;  // Якщо перевірка успішна - рухаємо комірника
      _y_global = y;
    }
  }
}  // namespace sokoban
