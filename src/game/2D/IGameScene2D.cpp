#include "IGameScene2D.h"
#pragma GCC optimize("O3")
#include <algorithm>

namespace pixeler
{
  static const uint8_t SCENE_TASK_QUEUE_DEPTH{20};

  uint32_t IGameScene2D::_obj_id_counter = 0;

  IGameScene2D::IGameScene2D(DataStream& stored_objs)
      : _terrain{TerrainManager2D()},
        _stored_objs{stored_objs},
        _task_queue{xQueueCreate(SCENE_TASK_QUEUE_DEPTH, sizeof(std::function<void()>*))}

  {
    if (!_task_queue)
    {
      log_e("Не вдалося створити _task_queue");
      esp_restart();
    }

    _owner_task_handle = xTaskGetCurrentTaskHandle();

    _obj_id_counter = 0;
  }

  IGameScene2D::~IGameScene2D()
  {
    _is_alive = false;

    for (auto const& obj : _game_objs)
      delete obj;

    delete _game_UI;
    delete _game_menu;

    std::function<void()>* task = nullptr;
    while (xQueueReceive(_task_queue, &task, 0) == pdTRUE)
      delete task;

    vQueueDelete(_task_queue);
  }

  void IGameScene2D::update()
  {
    if (_is_paused) [[unlikely]]
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

    processPostedTasks();

    _terrain.setCameraPos(_main_obj->_x_global, _main_obj->_y_global);
    _terrain.onDraw();

    std::vector<IGameObject2D*> view_objs;
    view_objs.reserve(_game_objs.size());
    IGameObject2D* obj;

    for (size_t i = 0; i < _game_objs.size();)
    {
      obj = _game_objs[i];

      if (!obj->_is_alive)
      {
        delete obj;
        _game_objs[i] = _game_objs.back();
        _game_objs.pop_back();
        continue;
      }

      ++i;

      obj->__update();

      if (obj->_is_triggered) [[unlikely]]
      {
        obj->_is_triggered = false;
        onTriggered(obj->_trigger_ID);
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

    std::sort(view_objs.begin(), view_objs.end(), [](IGameObject2D* a, IGameObject2D* b)
              {
        if (a->_layer != b->_layer)
          return a->_layer < b->_layer;
        return a->_y_global + a->_geometry->height < b->_y_global + b->_geometry->height; });

    for (auto const& game_obj : view_objs)
      game_obj->__onDraw();

    if (_game_UI)
      _game_UI->onDraw();
  }

  bool IGameScene2D::post(std::function<void()> task, unsigned long timeout_ms)
  {
    if (!_is_alive)
    {
      log_e("Спроба виконати post в мертвій сцені");
      esp_restart();
    }

    if (xTaskGetCurrentTaskHandle() == _owner_task_handle)
    {
      task();
      return true;
    }

    // Виділяємо копію в купі, бо queue копіює лише вказівник
    auto* task_ptr = new std::function<void()>(std::move(task));
    if (xQueueSend(_task_queue, &task_ptr, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    {
      delete task_ptr;
      if (timeout_ms > 0)
        log_e("Черга post переповнена, гра може працювати нестабільно");

      return false;
    }

    return true;
  }

  void IGameScene2D::processPostedTasks()
  {
    std::function<void()>* task = nullptr;
    uint32_t processed_count{0};

    while (xQueueReceive(_task_queue, &task, 0) == pdTRUE)
    {
      (*task)();
      delete task;

      if ((++processed_count & 15) == 0)
        delay(1);
    }
  }

  bool IGameScene2D::isFinished() const
  {
    return _is_finished;
  }

  bool IGameScene2D::isReleased() const
  {
    return _is_released;
  }

  uint8_t IGameScene2D::getNextSceneID() const
  {
    return _next_scene_ID;
  }

  void IGameScene2D::openSceneByID(uint16_t scene_ID)
  {
    _input.reset();
    _next_scene_ID = scene_ID;
    _is_released = true;
  }

  size_t IGameScene2D::calcObjectsSize() const
  {
    size_t sum{0};
    for (auto const& obj : _game_objs)
      sum += obj->getDataSize();
    return sum;
  }

  void IGameScene2D::serializeObjects(DataStream& ds) const
  {
    for (auto const& obj : _game_objs)
      obj->serialize(ds);

    ds.flush();
  }

  const SpriteTemplate* IGameScene2D::registerSpriteTemplate(uint16_t type_ID, SpriteTemplate tmpl)
  {
    auto [it, inserted] = _sprite_templates.emplace(type_ID, tmpl);

    if (!inserted)
    {
      log_e("Спроба повторної реєстрації шаблону спрайта для type_ID %u", type_ID);
      esp_restart();
    }

    return &it->second;
  }

  const SpriteTemplate* IGameScene2D::getSpriteTemplate(uint16_t type_ID) const
  {
    auto it = _sprite_templates.find(type_ID);

    if (it == _sprite_templates.end())
    {
      log_e("Не зареєстровано шаблон спрайта для type_ID %u", type_ID);
      esp_restart();
    }

    return &it->second;
  }

  std::vector<IGameObject2D*> IGameScene2D::getObjByType(std::span<const uint16_t> type_ID, const IGameObject2D* exclude)
  {
    std::vector<IGameObject2D*> ret_objs;
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

  std::vector<IGameObject2D*> IGameScene2D::getObjByTypeAt(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, const IGameObject2D* exclude)
  {
    std::vector<IGameObject2D*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude && obj->hasIntersectWithPoint(x, y))
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

  std::vector<IGameObject2D*> IGameScene2D::getObjByTypeInRect(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const IGameObject2D* exclude)
  {
    std::vector<IGameObject2D*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude && obj->hasIntersectWithRect(x, y, width, height))
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

  std::vector<IGameObject2D*> IGameScene2D::getObjByTypeInCircle(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, uint16_t radius, const IGameObject2D* exclude)
  {
    std::vector<IGameObject2D*> ret_objs;
    ret_objs.reserve(10);

    for (auto const& obj : _game_objs)
    {
      if (obj != exclude && obj->hasIntersectWithCircle(x, y, radius))
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

  bool IGameScene2D::hasCollisionAt(uint16_t x, uint16_t y, const IGameObject2D* exclude)
  {
    for (auto const& obj : _game_objs)
    {
      if (obj != exclude && obj->_physics.is_rigid && obj->hasIntersectWithPoint(x, y))
        return true;
    }

    return false;
  }

  bool IGameScene2D::canPass(const IGameObject2D& caller, uint16_t x_to, uint16_t y_to)
  {
    return _terrain.canPass(caller._x_global, caller._y_global, x_to, y_to, caller._physics, *caller._geometry);
  }

  void IGameScene2D::addObject(IGameObject2D& obj)
  {
    _game_objs.emplace_back(&obj);
  }

  void IGameScene2D::onTriggered(uint16_t trigg_id)
  {
    log_i("Викликано тригер: %u", trigg_id);
  }
}  // namespace pixeler
