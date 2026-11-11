#pragma once
#pragma GCC optimize("O3")
#include <unordered_map>

#include "../sprite/PhysicsState.h"
#include "../sprite/SpriteGeometry.h"
#include "Tile.h"
#include "defines.h"
#include "driver/graphics/DisplayWrapper.h"

namespace pixeler
{
  class IGameScene2D;

  class Terrain2D
  {
  public:
    struct ParallaxLayer
    {
      const uint16_t* img_data;

      float scroll_ratio;  // Чутливість до руху камери
      float speed_x;       // Швидкість горизонтального авторуху
      float accum_x;       // Акумулятор авторуху

      uint16_t width;
      uint16_t height;

      int16_t offset_x;
      int16_t offset_y;

      bool repeat_x;

      /**
       * @brief Фабрика для статичного шару, який завжди знаходиться в одному місці екрана.
       *
       * @param img_data Дані зображення
       * @param width Ширина зображення
       * @param height Висота зображення
       * @param offset_x Початкове зміщення від лівого краю виду
       * @param offset_y Початкове зміщення від верхнього краю видуd
       * @return ParallaxLayer
       */
      static constexpr ParallaxLayer makeStatic(const uint16_t* img_data,
                                                uint16_t width,
                                                uint16_t height,
                                                int16_t offset_x = 0,
                                                int16_t offset_y = 0)
      {
        return ParallaxLayer(img_data, width, height, offset_x, offset_y, 0.0f, 0.0f, false);
      }

      /**
       * @brief Фабрика для шару з автоматичним горизонтальним рухом, наприклад, хмари.
       *
       * @param img_data Дані зображення
       * @param width Ширина зображення
       * @param height Висота зображення
       * @param spd_x Швидкість руху зображення за кадр
       * @param offset_x Початкове зміщення від лівого краю виду
       * @param offset_y Початкове зміщення від верхнього краю виду
       * @param scroll_ratio Чутливість до руху камери
       * @return ParallaxLayer
       */
      static constexpr ParallaxLayer makeAutoScroll(const uint16_t* img_data,
                                                    uint16_t width,
                                                    uint16_t height,
                                                    float spd_x,
                                                    int16_t offset_x = 0,
                                                    int16_t offset_y = 0,
                                                    float scroll_ratio = 0.0f)
      {
        return ParallaxLayer(img_data, width, height, offset_x, offset_y, scroll_ratio, spd_x, true);
      }

      /**
       * @brief Фабрика для шару з класичним паралаксом, наприклад, гори або ліс на фоні.
       *
       * @param img_data Дані зображення
       * @param width Ширина зображення
       * @param height Висота зображення
       * @param ratio Чутливість до руху камери
       * @param offset_x Початкове зміщення від лівого краю виду
       * @param offset_y Початкове зміщення від верхнього краю виду
       * @param loop_x Прапор, що вказує на зациклення відрисовки шару
       * @return ParallaxLayer
       */
      static constexpr ParallaxLayer makeParallax(const uint16_t* img_data,
                                                  uint16_t width,
                                                  uint16_t height,
                                                  float ratio,
                                                  int16_t offset_x = 0,
                                                  int16_t offset_y = 0,
                                                  bool loop_x = true)
      {
        return ParallaxLayer(img_data, width, height, offset_x, offset_y, ratio, 0.0f, loop_x);
      }

    private:
      constexpr ParallaxLayer(const uint16_t* data,
                              uint16_t w,
                              uint16_t h,
                              int16_t off_x,
                              int16_t off_y,
                              float ratio,
                              float spd_x,
                              bool rep_x)
          : img_data(data),
            scroll_ratio(ratio),
            speed_x(spd_x),
            accum_x(0.0f),
            width(w),
            height(h),
            offset_x(off_x),
            offset_y(off_y),
            repeat_x(rep_x)
      {
        if (!img_data || width == 0 || height == 0)
        {
          log_e("Некоректні параметри паралакс-шару");
          esp_restart();
        }
      }
    };

//
#ifdef GRAPHICS_ENABLED
    Terrain2D() : VIEW_W{UI_WIDTH},
                         VIEW_H{UI_HEIGHT},
                         HALF_VIEW_W{static_cast<uint16_t>(UI_WIDTH * 0.5f)},
                         HALF_VIEW_H{static_cast<uint16_t>(UI_HEIGHT * 0.5f)} {}
#else
    // Заглушка, щоб не видаляти рушій, якщо графіку вимкнуто
    Terrain2D() : VIEW_W{10},
                         VIEW_H{10},
                         HALF_VIEW_W{static_cast<uint16_t>(5)},
                         HALF_VIEW_H{static_cast<uint16_t>(5)} {}
#endif  // #ifdef GRAPHICS_ENABLED
    ~Terrain2D();

    /**
     * @brief Додає новий шар з зображенням, який буде відмальовуватися
     * раніше за зображення плиток та ігрових об'єктів.
     *
     * @param layer
     */
    void addParallaxLayer(const ParallaxLayer& layer);

    /**
     * @brief Очищує інформацію про фонові зображення
     *
     */
    void clearParallaxLayers();

    /**
     * @brief Встановлює колір заливки фону сцени
     *
     * @param color
     */
    void setBackColor(uint16_t color);

    /**
     * @brief Повертає колір заливки фону сцени
     *
     * @return uint16_t
     */
    uint16_t getBackColor() const;

    /**
     * @brief Вмикає/вимикає заливку сцени фоновим кольором на 0-вому етапі формування сцени.
     * За замовченням увімкнено.
     *
     * @param state
     */
    void setBackColorFilling(bool state);

    /**
     * @brief Додає опис окремої плитки.
     * Дані за вказівником sprite_src не будуть видалені автоматично.
     *
     * @param sprite_id Ідентифікатор плитки.
     * @param type Тип прохідності плитки.
     * @param sprite_src Зображення плитки.
     */
    void addTileInfo(uint16_t sprite_id, TileType type, const uint16_t* sprite_src);

    /**
     * @brief Скидає опис плиток.
     * Необхідно викликати перед додаванням нових плиток, якщо потрібно перемалювати ігровий рівень в межах однії сцени.
     *
     */
    void clearTilesInfos();

    /**
     * @brief Створює поверхню ігрового рівня на основі опису плиток та таблиці розміщення плиток.
     * Ігровий рівень повиннен бути не меншим за розмір дисплея, в іншому разі, під час малювання ігрового рівня, виникне помилка.
     * Опис плиток повинен бути доданий до виклику цього методу.
     *
     * @param tiles_w_num Кількість плиток в ширину.
     * @param tiles_h_num Кількість плиток у висоту.
     * @param tile_side_len Розмір сторони плитки. Плитки повинні мати квадратну форму.
     * @param tiles_pos_tmpl Вказівник на масив, що містить шаблон ігрової сцени.
     */
    void build(uint16_t tiles_w_num,
               uint16_t tiles_h_num,
               uint16_t tile_side_len,
               const uint16_t* tiles_pos_tmpl);

    /**
     * @brief Перевіряє, чи може об'єкт з вказаним спрайтом бути переміщеним в указані координати ігрового рівня.
     *
     * @param x_from Х-координата точки, звідки повинне виконуватися переміщення.
     * @param y_from Y-координата точки, звідки повинне виконуватися переміщення.
     * @param x_to Х-координата точки, куди повинне виконуватися переміщення.
     * @param y_to Y-координата точки, куди повинне виконуватися переміщення.
     * @param physics Стан фізичних фластивостей ігрового об'єкта.
     * @param geometry Геометрія спрайта ігрового об'єкта.
     * @return true - Якщо об'єкт може бути переміщено в указані координатах ігрового рівня. false - інакше.
     */
    bool canPass(uint16_t x_from,
                 uint16_t y_from,
                 uint16_t x_to,
                 uint16_t y_to,
                 const PhysicsState& physics,
                 const SpriteGeometry& geometry) const;

    /**
     * @brief Повертає тип плитки у вказаних координатах.
     *
     * @param x Координата плитки.
     * @param y Координата плитки.
     * @return TileType тип плитки у вказаних координатах. TILE_TYPE_NONE - якщо плитка за вказаними координатами відсутня.
     */
    TileType getTileType(uint16_t x, uint16_t y) const;

  private:
    friend class IGameScene2D;

    /**
     * @brief Малює частину ігрового рівня з указаних координат, якщо можливо.
     * Інакше малює частину ігрового рівня найближчу до цих координат.
     *
     */
    void onDraw();

    /**
     * @brief Перевіряє чи потрапляє хоча б частина спрайту на зображення кадру.
     *
     * @param x_pos X - Координата спрайта.
     * @param y_pos Y - Координата спрайта.
     * @param sprite_w Ширина спрайта.
     * @param sprite_h Висота спрайта.
     * @return true - Якщо спрайт частково чи повністю потрапляє на зображення кадру. false - інакше.
     */
    bool isInView(uint16_t x_pos, uint16_t y_pos, uint16_t sprite_w, uint16_t sprite_h) const;

    /**
     * @brief Встановлює позицію камери за вказаними координатами.
     *
     * @param x Координата камери.
     * @param y Координата камери.
     */
    void setCameraPos(uint16_t x, uint16_t y);

    void renderParallax();

    void freeMem();
    void freeTilesDescriptionData();

    uint16_t coordToTilePos(uint16_t coord) const;

  private:
    std::unordered_map<uint16_t, Tile*> _tile_descr;  // Опис плиток
    std::vector<ParallaxLayer> _parallax_layers;      // Опис фонових зображень
    Tile*** _terrain{nullptr};                        // Набір плиток ігрового рівня
    const uint16_t VIEW_W;                            // Ширина view-порта
    const uint16_t VIEW_H;                            // Висота view-порта
    const uint16_t HALF_VIEW_W;                       // Використовується для позиціонування об'єктів
    const uint16_t HALF_VIEW_H;                       // Використовується для позиціонування об'єктів
    uint16_t _back_color{COLOR_BLACK};                // Колір фонової заливки
    uint16_t _view_x{0};                              // X верхнього лівого кута view-порта
    uint16_t _view_y{0};                              // Y верхнього лівого кута view-порта
    uint16_t _width{0};                               // Ширина ігрового рівня в пікселях
    uint16_t _height{0};                              // Висота ігрового рівня в пікселях
    uint16_t _tile_x_num{0};                          // Кількість плиток мапи по горизонталі
    uint16_t _tile_y_num{0};                          // Кількість плиток мапи по вертикалі
    uint8_t _tile_side_len{0};                        // Розмір сторони плитки

    bool _has_back_color{true};
  };
}  // namespace pixeler
