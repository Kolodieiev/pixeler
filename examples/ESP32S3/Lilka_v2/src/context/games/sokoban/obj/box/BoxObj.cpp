#include "BoxObj.h"

#include "../../../common_res/box_img/sprite_box.h"
#include "../../../common_res/box_img/sprite_box_ok.h"
#include "../../ResID.h"
#include "../TypeID.h"
#include "game/IGameScene.h"

namespace sokoban
{
  BoxObj::BoxObj(uint32_t id, IGameScene& game_scene, SfxPlayer& audio) : IGameObject(id, TYPE_BOX, game_scene, audio)
  {
    _layer = 1;  // Об'єкт повинен бути вище об'єктів точок щоб перекривати їх
    _sprite.img_data = SPRITE_BOX;
    _sprite.has_img = true;
    _sprite.width = 32;
    _sprite.height = 32;
    _sprite.pass_abillity_mask = TILE_TYPE_GROUND;
  }

  void BoxObj::__update()
  {
  }

  void BoxObj::serialize(DataStream& ds) const
  {
  }

  void BoxObj::deserialize(DataStream& ds)
  {
  }

  size_t BoxObj::getDataSize() const
  {
    return 0;
  }

  bool BoxObj::moveTo(uint16_t x, uint16_t y)
  {
    bool has_point = false;  // Прапор який вказує, чи маємо ключову точку в координатах переміщення

    const std::array types = {(uint16_t)TYPE_BOX, (uint16_t)TYPE_BOX_POINT};

    // Вибрати об'єкти ящиків та ключових точок на плитці куди повинен бути встановлений ящик
    std::vector<IGameObject*> objs = _scene.getObjByTypeAt(types, x, y, this);

    for (auto it = objs.begin(), last_it = objs.end(); it != last_it; ++it)
    {
      if ((*it)->getTypeID() == TYPE_BOX)  // Якщо знайдено об'єкт ящика, рух продовжувати не можна
        return false;

      if ((*it)->getTypeID() == TYPE_BOX_POINT)  // Якщо об'єкт належить до типу BoxPointObj
        has_point = true;                        // Підіймаємо прапор, але не перериваємо цикл, щоб переконатися у відсутності ящика
    }

    if (!has_point)  // Якщо точку не знайдено значить далі або прохід або стіна.
    {
      _is_ok = false;
      _sprite.img_data = SPRITE_BOX;
      // Перевіряємо чи не впираємося в стіну за ящиком
      if (!_scene.canPass(*this, x, y))
        return false;  // Якщо впираємося в стіну, рух продовжувати не можна
    }
    else
    {
      _is_ok = true;                     // Підняти прапор, який вказує, що ящик встановлено в потрібному місці
      _sprite.img_data = SPRITE_BOX_OK;  // Змінити спрайт об'єкта
    }

    // Якщо всі перевірки пройдено переміщуємо цей об'єкт ящика на нову плитку
    _x_global = x;
    _y_global = y;

    // Відтворення звуку по його ідентфікатору //TODO переписати відповідно до нового менеджера ресурсів

    // int start = 0;
    // int end = 4;

    // // Отримати випадковий звук переміщення
    // uint8_t track_ID = rand() % (end - start + 1) + start;
    // WavData *s_data = _res_manager.getWav(track_ID);

    // if (s_data && s_data->size > 0)
    // {
    //     SFX *track = new SFX(s_data->data_ptr, s_data->size);
    //     track->setVolume(4);

    //     if (!track)
    //     {
    //         log_e("Помилка виділення пам'яті");
    //         esp_restart();
    //     }

    //     _sfx_player.addSFX(track);
    // }
    // else
    // {
    //     log_e("Звукові дані з таким ID{%d} відсутні", track_ID);
    // }

    return true;
  }
}  // namespace sokoban
