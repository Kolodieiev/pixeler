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
    if (_is_paused)
    {
      if (_game_menu)
        _game_menu->onDraw();

      return;
    }

    if (!_main_obj)
    {
      log_e("Не встановлено головний ігровий об'єкт");
      esp_restart();
    }

    takeLock();

    _terrain.setCameraPos(_main_obj->_x_global, _main_obj->_y_global);
    _terrain.onDraw();

    std::vector<IGameObject*> view_obj;
    view_obj.reserve(_game_objs.size());
    IGameObject* obj;

    for (auto it = _game_objs.begin(), last_it = _game_objs.end(); it != last_it;)
    {
      obj = *it;

      if (!obj->isDestroyed())
      {
        obj->__update();

        if (obj->_is_triggered)
        {
          obj->_is_triggered = false;
          giveLock();
          onTriggered(obj->_trigger_ID);
          takeLock();
        }

        if (_terrain.isInView(obj->_x_global, obj->_y_global, obj->_sprite.width, obj->_sprite.height))
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
          view_obj.push_back(obj);
        }

        ++it;
      }
      else
      {
        delete obj;
        it = _game_objs.erase(it);
      }
    }

    std::sort(view_obj.begin(), view_obj.end(), [](IGameObject* a, IGameObject* b)
              {
    if (a->_layer < b->_layer) 
        return true;
    return a->_y_global + a->_sprite.height < b->_y_global + b->_sprite.height; });

    for (auto const& g_obj : view_obj)
      g_obj->__onDraw();

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

  void IGameScene::takeLock()
  {
    xSemaphoreTake(_obj_mutex, portMAX_DELAY);
  }

  void IGameScene::giveLock()
  {
    xSemaphoreGive(_obj_mutex);
  }

  void IGameScene::openSceneByID(uint16_t scene_ID)
  {
    _input.reset();
    _next_scene_ID = scene_ID;
    _is_released = true;
  }

  size_t IGameScene::getObjsSize()
  {
    size_t sum{0};
    takeLock();
    for (auto const& obj : _game_objs)
      sum += obj->getDataSize();
    giveLock();
    return sum;
  }

  void IGameScene::serialize(DataStream& ds)
  {
    takeLock();
    for (auto const& obj : _game_objs)
      obj->serialize(ds);
    giveLock();

    ds.flush();
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
      if (obj != exclude && obj->_sprite.is_rigid && obj->hasIntersectWithPoint(x, y))
        return true;
    }

    return false;
  }

  bool IGameScene::canPass(const IGameObject& caller, uint16_t x_to, uint16_t y_to)
  {
    return _terrain.canPass(caller._x_global, caller._y_global, x_to, y_to, caller._sprite);
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
