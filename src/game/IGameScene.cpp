#include "IGameScene.h"
#pragma GCC optimize("O3")
#include <algorithm>

namespace pixeler
{
  uint32_t IGameScene::_obj_id_counter = 0;

  IGameScene::IGameScene(DataStream& stored_objs) : _terrain{TerrainManager()},
                                                    _stored_objs{stored_objs},
                                                    _obj_mutex{xSemaphoreCreateMutex()}
  {
    if (!_obj_mutex)
    {
      log_e("Не вдалося створити _obj_mutex");
      esp_restart();
    }

    _obj_id_counter = 0;
  }

  IGameScene::~IGameScene()
  {
    for (auto const& obj : _game_objs)
      delete obj;

    delete _game_UI;
    delete _game_menu;
  }

  void IGameScene::update()
  {
    if (_is_paused) [[unlikely]]
    {
      if (_game_menu)
        _game_menu->onDraw();

      return;
    }

    if (!_main_obj) [[unlikely]]
    {
      log_e("Не встановлено головний ігровий об'єкт");
      esp_restart();
    }

    takeLock();

    _terrain.setCameraPos(_main_obj->_x_global, _main_obj->_y_global);
    _terrain.onDraw();

    std::vector<IGameObject*> view_objs;
    view_objs.reserve(_game_objs.size());
    IGameObject* obj;

    for (size_t i = 0; i < _game_objs.size(); ++i)
    {
      obj = _game_objs[i];

      if (!obj->_is_alive) [[unlikely]]
      {
        delete obj;
        _game_objs[i] = _game_objs.back();
        _game_objs.pop_back();
        continue;
      }

      obj->__update();

      if (obj->_is_triggered) [[unlikely]]
      {
        obj->_is_triggered = false;
        giveLock();
        onTriggered(obj->_trigger_ID);
        takeLock();
      }

      if (obj->_sprite.has_img || obj->_sprite.has_animation)
      {
        if (_terrain.isInView(obj->_x_global, obj->_y_global, obj->_geometry->width, obj->_geometry->height))
        {
          if (obj != _main_obj)
          {
            obj->_x_local = obj->_x_global - _terrain.getViewX();
            obj->_y_local = obj->_y_global - _terrain.getViewY();
          }
          else
          {
            if (_main_obj->_x_global < _terrain.HALF_VIEW_W)
              _main_obj->_x_local = _main_obj->_x_global;
            else if (_main_obj->_x_global < _terrain.getWidth() - _terrain.HALF_VIEW_W)
              _main_obj->_x_local = _terrain.HALF_VIEW_W;
            else
              _main_obj->_x_local = _terrain.VIEW_W + _main_obj->_x_global - _terrain.getWidth();

            if (_main_obj->_y_global < _terrain.HALF_VIEW_H)
              _main_obj->_y_local = _main_obj->_y_global;
            else if (_main_obj->_y_global < _terrain.getHeight() - _terrain.HALF_VIEW_H)
              _main_obj->_y_local = _terrain.HALF_VIEW_H;
            else
              _main_obj->_y_local = _terrain.VIEW_H + _main_obj->_y_global - _terrain.getHeight();
          }
          view_objs.push_back(obj);
        }
      }
    }

    std::sort(view_objs.begin(), view_objs.end(), [](IGameObject* a, IGameObject* b)
              {
    if (a->_layer < b->_layer) 
        return true;
    return a->_y_global + a->_geometry->height < b->_y_global + b->_geometry->height; });

    for (auto const& game_obj : view_objs)
      game_obj->__onDraw();

    giveLock();

    if (_game_UI)
      _game_UI->onDraw();
  }

  bool IGameScene::isFinished() const
  {
    return _is_finished;
  }

  bool IGameScene::isReleased() const
  {
    return _is_released;
  }

  uint8_t IGameScene::getNextSceneID() const
  {
    return _next_scene_ID;
  }

  void IGameScene::takeLock() const
  {
    xSemaphoreTake(_obj_mutex, portMAX_DELAY);
  }

  void IGameScene::giveLock() const
  {
    xSemaphoreGive(_obj_mutex);
  }

  void IGameScene::openSceneByID(uint16_t scene_ID)
  {
    _input.reset();
    _next_scene_ID = scene_ID;
    _is_released = true;
  }

  size_t IGameScene::calcObjectsSize() const
  {
    size_t sum{0};
    takeLock();
    for (auto const& obj : _game_objs)
      sum += obj->getDataSize();
    giveLock();
    return sum;
  }

  void IGameScene::serializeObjects(DataStream& ds) const
  {
    takeLock();
    for (auto const& obj : _game_objs)
      obj->serialize(ds);
    giveLock();

    ds.flush();
  }

  const SpriteTemplate* IGameScene::registerSpriteTemplate(uint16_t type_ID, SpriteTemplate tmpl)
  {
    auto [it, inserted] = _sprite_templates.emplace(type_ID, tmpl);

    if (!inserted)
    {
      log_e("Спроба повторної реєстрації шаблону спрайта для type_ID %u", type_ID);
      esp_restart();
    }

    return &it->second;
  }

  const SpriteTemplate* IGameScene::getSpriteTemplate(uint16_t type_ID) const
  {
    auto it = _sprite_templates.find(type_ID);

    if (it == _sprite_templates.end())
    {
      log_e("Не зареєстровано шаблон спрайта для type_ID %u", type_ID);
      esp_restart();
    }

    return &it->second;
  }

  std::vector<IGameObject*> IGameScene::getObjByType(std::span<const uint16_t> type_ID, const IGameObject* exclude)
  {
    std::vector<IGameObject*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude)
      {
        for (const uint16_t id : type_ID)
        {
          if (obj->_type_ID == id)
          {
            ret_objs.push_back(obj);
            break;
          }
        }
      }
    }

    return ret_objs;
  }

  std::vector<IGameObject*> IGameScene::getObjByTypeAt(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, const IGameObject* exclude)
  {
    std::vector<IGameObject*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude)
      {
        for (const uint16_t id : type_ID)
        {
          if (obj->_type_ID == id && obj->hasIntersectWithPoint(x, y))
          {
            ret_objs.push_back(obj);
            break;
          }
        }
      }
    }

    return ret_objs;
  }

  std::vector<IGameObject*> IGameScene::getObjByTypeInRect(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const IGameObject* exclude)
  {
    std::vector<IGameObject*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude)
      {
        for (const uint16_t id : type_ID)
        {
          if (obj->_type_ID == id && obj->hasIntersectWithRect(x, y, width, height))
          {
            ret_objs.push_back(obj);
            break;
          }
        }
      }
    }

    return ret_objs;
  }

  std::vector<IGameObject*> IGameScene::getObjByTypeInCircle(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, uint16_t radius, const IGameObject* exclude)
  {
    std::vector<IGameObject*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude)
      {
        for (const uint16_t id : type_ID)
        {
          if (obj->_type_ID == id && obj->hasIntersectWithCircle(x, y, radius))
          {
            ret_objs.push_back(obj);
            break;
          }
        }
      }
    }

    return ret_objs;
  }

  bool IGameScene::hasCollisionAt(uint16_t x, uint16_t y, const IGameObject* exclude)
  {
    for (auto const& obj : _game_objs)
    {
      if (obj != exclude && obj->_physics.is_rigid && obj->hasIntersectWithPoint(x, y))
        return true;
    }

    return false;
  }

  bool IGameScene::canPass(const IGameObject& caller, uint16_t x_to, uint16_t y_to)
  {
    return _terrain.canPass(caller._x_global, caller._y_global, x_to, y_to, caller._physics, *caller._geometry);
  }

  void IGameScene::addObject(IGameObject& obj)
  {
    _game_objs.emplace_back(&obj);
  }

  void IGameScene::onTriggered(uint16_t trigg_id)
  {
    log_i("Викликано тригер: %u", trigg_id);
  }
}  // namespace pixeler
