#pragma once
#pragma GCC optimize("O3")

#include "DataStream.h"
#include "SpriteDescription.h"
#include "defines.h"
#include "driver/graphics/DisplayWrapper.h"
#include "manager/ResManager.h"
#include "sound/SfxPlayer.h"
#include "terrain/TileType.h"

namespace pixeler
{
  class IGameScene;

  class IGameObject
  {
  public:
    enum MovingDirection : uint8_t
    {
      DIRECTION_NONE,
      DIRECTION_UP,
      DIRECTION_DOWN,
      DIRECTION_LEFT,
      DIRECTION_RIGHT
    };

    IGameObject(const IGameObject& rhs) = delete;
    IGameObject& operator=(const IGameObject& rhs) = delete;

    IGameObject(uint32_t id, uint16_t type_id, IGameScene& game_scene, SfxPlayer& audio);
    virtual ~IGameObject() = 0;

    /**
     * @brief Повертає реальний розмір даних об'єкта, які будуть серіалізовані.
     *
     * @return size_t
     */
    virtual size_t getDataSize() const = 0;

    /**
     * @brief Серіалізує об'єкт в DataStream
     *
     * @param ds Об'єкт DataStream, куди буде серіалізовано дані.
     */
    virtual void serialize(DataStream& ds) const = 0;

    /**
     * @brief Десеріаізує об'єкт із DataStream
     *
     * @param ds Об'єкт DataStream, з якого будуть прочитані дані у відповідні поля об'єкта.
     */
    virtual void deserialize(DataStream& ds) = 0;

    /**
     * @brief Повертає ідентифікатор об'єкта.
     *
     * @return uint32_t
     */
    uint32_t getID() const;

    /**
     * @brief Повертає ідентифікатор типу об'єкта.
     * Необхідний для заміни instanceof.
     *
     * @return uint16_t
     */
    uint16_t getTypeID() const;

    /**
     * @brief Повертає вказівник на рядок з ім'ям об'єкта, якщо було задано. Інакше на порожій рядок.
     *
     * @return const char*
     */
    const char* getName() const;

    /**
     * @brief Встановлює глобальну позицію об'єкта на ігровій сцені.
     *
     * @param x_pos Координата.
     * @param y_pos Координата.
     */
    void setPos(uint16_t x_pos, uint16_t y_pos);

    /**
     * @brief Повертає глобальну x-координату об'єкта на ігровому рівні.
     *
     * @return uint16_t
     */
    uint16_t getGlobalX() const;

    /**
     * @brief Повертає глобальну у-координату об'єкта на ігровому рівні.
     *
     * @return uint16_t
     */
    uint16_t getGlobalY() const;

    /**
     * @brief Повертає стан прапору, який вказує чи було об'єкт знищено в попередньому кадрі.
     * Зазвичай використовується сценою для очищення мертвих об'єктів з ігрового рівня.
     * Може бути використаний для запобігання взаємодії з потенційно мертвим об'єктом.
     *
     * @return true - Якщо об'єкт потенційно мертвий. false - Інакше.
     */
    bool isDestroyed() const;

  protected:
    /**
     * @brief Забезпечує оновлення стану об'єкта. Викликається автоматично ігровим рівнем.
     *
     */
    virtual void __update() = 0;

    /**
     * @brief Перемальовує об'єкт кожен кадр, якщо у нього задано зображення або анімацію.
     *
     */
    virtual void __onDraw();

    /**
     * @brief Перевіряє чи пересікається об'єкт з заданою точкою.
     *
     * @param x Координата точки.
     * @param y Координата точки.
     * @param rigid_only Задає тип перевірки. Якщо true буде перевірятися тільки пересічення з жорстким тілом. Інакше зі спрайтом.
     * @return true - Якщо пересікається.
     * @return false - Інакше.
     */
    bool hasIntersectWithPoint(uint16_t x, uint16_t y, bool rigid_only = false) const;

    /**
     * @brief Перевіряє чи пересікається об'єкт з заданим колом.
     *
     * @param x_mid Центральна координата кола.
     * @param y_mid Центральна координата кола.
     * @param radius Радіус кола.
     * @param rigid_only Задає тип перевірки. Якщо true буде перевірятися тільки пересічення з жорстким тілом. Інакше зі спрайтом.
     * @return true - Якщо пересікається.
     * @return false - Інакше.
     */
    bool hasIntersectWithCircle(uint16_t x_mid, uint16_t y_mid, uint16_t radius, bool rigid_only = false) const;

    /**
     * @brief Перевіряє чи пересікається об'єкт з заданим прямокутником.
     *
     * @param x_start Координата верхнього лівого кута прямокутника.
     * @param y_start Координата верхнього лівого кута прямокутника.
     * @param rect_width Ширина прямокутника.
     * @param rect_height Висота прямокутника.
     * @param rigid_only Задає тип перевірки. Якщо true буде перевірятися тільки пересічення з жорстким тілом. Інакше зі спрайтом.
     * @return true - Якщо пересікається.
     * @return false - Інакше.
     */
    bool hasIntersectWithRect(uint16_t x_start, uint16_t y_start, uint16_t rect_width, uint16_t rect_height, bool rigid_only = false) const;

    /**
     * @brief Розраховує кут від pivot об'єкта, до вказаної точки.
     *
     * @param x Координата точки.
     * @param y Координата точки.
     * @return uint16_t
     */
    uint16_t calcAngleToPoint(uint16_t x, uint16_t y);

    /**
     * @brief Розраховує відстань від pivot об'єкта до вказаної точки.
     *
     * @param x Координата точки.
     * @param y Координата точки.
     * @return uint16_t
     */
    uint16_t calcDistToPoint(uint16_t x, uint16_t y);

    /**
     * @brief Переміщує об'єкт на вказану кількість пікселів в сторону указаної точки.
     *
     * @param x_to Координата точки.
     * @param y_to Координата точки.
     * @param step_w Розмір кроку в пікселях.
     */
    void stepToPoint(uint16_t x_to, uint16_t y_to, uint16_t step_w);

  private:
    friend class IGameScene;

  protected:
    IGameScene& _scene;
    SfxPlayer& _sfx_player;       // Плеєр звукових ефектів
    SpriteDescription _sprite{};  // Об'єкт структури, яка описує спрайт об'єкта та його стани

    String _name;  // Ім'я об'єкта, може не використовуватися

  private:
    int32_t _x_local{0};  // Координата X відносно дисплея
    int32_t _y_local{0};  // Координата Y відносно дисплея

  protected:
    const uint32_t _obj_ID;  // Ідентифікатор об'єкта. Може не використовуватися в локальній грі.

    uint16_t _x_global{0};  // Координата Х відносно ігрового рівня
    uint16_t _y_global{0};  // Координата Y відносно ігрового рівня

    const uint16_t _type_ID{0};  // Ідентифікатор типу об'єкта
    uint16_t _trigger_ID{0};     // Ідентифіктор тригера. Може не використовуватися, якщо об'єкт не тригериться.

    uint8_t _layer{0};  // Шар сортування об'єкта по осі Z. Чим більше значення, тим вище шар

    bool _is_triggered{false};  // Прапор спрацювання тригера об'єкта.  Може не використовуватися, якщо об'єкт не тригериться.
    bool _is_destroyed{false};  // Прапор знищення об'єкта іншими об'єктами або сценою
  };
}  // namespace pixeler
