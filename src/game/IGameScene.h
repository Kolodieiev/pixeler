#pragma once
#pragma GCC optimize("O3")
#include "defines.h"
//
#include <array>
#include <span>
#include <vector>
//
#include "../driver/graphics/DisplayWrapper.h"
#include "../driver/input/Input.h"
#include "../manager/ResManager.h"
#include "sound/SfxPlayer.h"

//
#include "IGameObject.h"
#include "terrain/TerrainLoader.h"
#include "terrain/TerrainManager.h"
#include "ui/IGameMenu.h"
#include "ui/IGameUI.h"

namespace pixeler
{
  class IGameScene
  {
  public:
    IGameScene(DataStream& stored_objs);
    virtual ~IGameScene() = 0;

    /**
     * @brief Забезпечує оновлення стану гри.
     * Викликається контекстом кожен кадр.
     *
     */
    virtual void update() = 0;

    IGameScene(const IGameScene& rhs) = delete;
    IGameScene& operator=(const IGameScene& rhs) = delete;

    /**
     * @brief Повертає стан прапору, який вказує на те, чи повинна бути завершена гра.
     * Зазвичай, викликається керуючим контекстом.
     *
     * @return true - Якщо гра повинна бути завершена. false - Інакше.
     */
    bool isFinished() const;

    /**
     * @brief Повертає стан прапору, який вказує на те, чи повинен бути зміненений поточний ігровий рівень.
     * Зазвичай, викликається керуючим контекстом.
     *
     * @return true - Якщо ігровий рівень повинен бути змінений. false - Інакше.
     */
    bool isReleased() const;

    /**
     * @brief Повертає ідентифікатор ігрового рівня, який повинен бути запущений наступним.
     * Зазвичай, викликається керуючим контекстом.
     *
     * @return uint8_t
     */
    uint8_t getNextSceneID() const;

    /**
     * @brief Повертає список усіх об'єктів на сцені з заданими ідентифікаторами класів.
     *
     * @param type_ID Ідентифікатори типу.
     * @param exclude Вказівник на об'єкт, який буде пропущено під час вибірки.
     * @return std::vector<IGameObject*>
     */
    std::vector<IGameObject*> getObjByType(std::span<const uint16_t> type_ID, const IGameObject* exclude = nullptr);

    /**
     * @brief Повертає список усіх об'єктів, що знаходяться в указаній точці і мають відповідний ідентифікатор класу.
     *
     * @param type_ID Ідентифікатори типу.
     * @param x Координата точки.
     * @param y Координата точки.
     * @param exclude Вказівник на об'єкт, який буде пропущено під час вибірки.
     * @return std::vector<IGameObject*>
     */
    std::vector<IGameObject*> getObjByTypeAt(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, const IGameObject* exclude = nullptr);

    /**
     * @brief Повертає список усіх об'єктів, що пересікаються з заданим прямокутним і мають відповідний ідентифікатор класу.
     *
     * @param type_ID Ідентифікатори типу.
     * @param x Координата верхнього лівого кута прямокутника.
     * @param y Координата верхнього лівого кута прямокутника.
     * @param width Ширина прямокутника.
     * @param height Висота прямокутника.
     * @param exclude Вказівник на об'єкт, який буде пропущено під час вибірки.
     * @return std::vector<IGameObject*>
     */
    std::vector<IGameObject*> getObjByTypeInRect(std::span<const uint16_t> type_ID,
                                                 uint16_t x,
                                                 uint16_t y,
                                                 uint16_t width,
                                                 uint16_t height,
                                                 const IGameObject* exclude = nullptr);

    /**
     * @brief Повертає список усіх об'єктів, що пересікаються з заданим колом і мають відповідний ідентифікатор класу.
     *
     * @param type_ID Ідентифікатори типу.
     * @param x Координата центральної точки кола.
     * @param y Координата центральної точки кола.
     * @param radius Радіус кола, що обмежує вибірку.
     * @param exclude Вказівник на об'єкт, який буде пропущено під час вибірки.
     * @return std::vector<IGameObject*>
     */
    std::vector<IGameObject*> getObjByTypeInCircle(std::span<const uint16_t> type_ID, uint16_t x, uint16_t y, uint16_t radius, const IGameObject* exclude = nullptr);

    /**
     * @brief Перевіряє чи є у вказаних координатах будь-який об'єкт з твердим тілом.
     *
     * @param exclude
     * @param x Координата.
     * @param y Координата.
     * @return true - Якщо об'єкт матиме пересічення.
     * @return false - Інакше.
     */
    bool hasCollisionAt(uint16_t x, uint16_t y, const IGameObject* exclude = nullptr);

    /**
     * @brief Перевіряє, чи може об'єкт з вказаним спрайтом бути переміщеним в указані координати ігрового рівня.
     *
     * @param caller Об'єкт, для якого буде виконана перевірка.
     * @param x_to Х-координата точки, куди повинне виконуватися переміщення.
     * @param y_to Y-координата точки, куди повинне виконуватися переміщення.
     * @return true - Якщо об'єкт має право переміщуватися в указану точку.
     * @return false - Інакше.
     */
    bool canPass(const IGameObject& caller, uint16_t x_to, uint16_t y_to);

    /**
     * @brief Додає ігровий об'єкт до контейнера об'єктів сцени.
     *
     * @param obj Адреса ігрового об'єкта.
     */
    void addObject(IGameObject& obj);

    /**
     * @brief Створює об'єкт вказаного типу.
     *
     * @tparam T Тип об'єкта, який потрібно створити.
     * @return T* Вказівник на створений об'єкт.
     */
    template <typename T>
    T* createObject()
    {
      try
      {
        return new T(++_obj_id_counter, *this, _sfx_player);
      }
      catch (const std::bad_alloc& e)
      {
        log_e("%s", e.what());
        esp_restart();
      }
    }

  protected:
    /**
     * @brief Якщо не перевантажено, слугує заглушкою для тригерів об'єктів.
     * Інакше буде викликано у об'єкта спадкоємця.
     *
     * @param id Ідентифікатор тригера.
     */
    virtual void onTriggered(uint16_t trigg_id);

    /**
     * @brief Блокує мютекс доступу до ігрових об'єктів.
     *
     */
    void takeLock() const;

    /**
     * @brief Відпускає мютекс доступу до ігрових об'єктів.
     *
     */
    void giveLock() const;

    /**
     * @brief Піднімає прапор, який вказує, що поточний ігровий рівень повинен бути змінений.
     * Встановлює ідентифікатро ігрового рівня, який повинен бути створений наступним.
     *
     * @param scene_ID Ідентифікатор ігрового рівня, що повинен бути створений наступним.
     */
    void openSceneByID(uint16_t scene_ID);

    /**
     * @brief Повертає загальний розмір даних ігрових об'єктів, які можуть бути серіалізовані.
     *
     * @return size_t
     */
    size_t getObjsSize() const;

    /**
     * @brief Записує усі об'єкти на сцені в DataStream
     *
     * @param ds Об'єкт DataStream, куди будуть серіалізовані об'єкти сцени.
     */
    void serialize(DataStream& ds) const;

  protected:
    ResManager _res_manager;  // Менеджер ресурсів
    TerrainManager _terrain;  // Самий нижній шар сцени
    SfxPlayer _sfx_player;    // Плеєр звукових ефектів

  private:
    std::vector<IGameObject*> _game_objs;  // Список усіх ігрових об'єктів на сцені, які повинні взаємодіяти один з одним

  protected:
    DataStream& _stored_objs;              // Контейнер для перенесення відбитків об'єктів до наступної сцени
    mutable SemaphoreHandle_t _obj_mutex;  // Мютекс для синхронізації доступу до об'єктів
    IGameUI* _game_UI{nullptr};            // Шар ігрового UI. Тут можуть виводитися графічні елементи інтерфейса
    IGameMenu* _game_menu{nullptr};        // Шар ігрового меню, якщо в ньому є необхідність
    IGameObject* _main_obj{nullptr};       // Об'єкт, за яким завжди слідує камера

  private:
    static uint32_t _obj_id_counter;  // Глобальний лічильник ідентифікаторів об'єктів.

  protected:
    uint8_t _next_scene_ID{0};  // Ідентифікатор наступної сцени

    bool _is_paused{false};    // Прапор встановлення сцени на паузу
    bool _is_released{false};  // Прапор, який вказує, що поточна сцена готова звільнити своє місце для наступної сцени
    bool _is_finished{false};  // Прапор, який повідомляє керуючому контексту, що гру завершено
  };
}  // namespace pixeler
