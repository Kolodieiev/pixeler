#pragma once
#pragma GCC optimize("O3")
#include <cmath>
#include <unordered_map>

#include "../../defines.h"
#include "../../driver/graphics/DisplayWrapper.h"
#include "../SpriteDescription.h"
#include "Tile.h"

namespace pixeler
{
  class TerrainManager
  {
  public:
//
#ifdef GRAPHICS_ENABLED
    TerrainManager() : VIEW_W{UI_WIDTH},
                       VIEW_H{UI_HEIGHT},
                       HALF_VIEW_W{static_cast<uint16_t>(UI_WIDTH * 0.5f)},
                       HALF_VIEW_H{static_cast<uint16_t>(UI_HEIGHT * 0.5f)} {}
#else
    // Заглушка, щоб не видаляти рушій, якщо графіку вимкнуто
    TerrainManager() : VIEW_W{10},
                       VIEW_H{10},
                       HALF_VIEW_W{static_cast<uint16_t>(5)},
                       HALF_VIEW_H{static_cast<uint16_t>(5)} {}
#endif  // #ifdef GRAPHICS_ENABLED
    ~TerrainManager();

    /**
     * @brief Повертає ширину ігрового рівня в пікселях.
     *
     * @return uint16_t
     */
    uint16_t getWidth() const;

    /**
     * @brief  Повертає висоту ігрового рівня в пікселях.
     *
     * @return uint16_t
     */
    uint16_t getHeight() const;

    /**
     * @brief Повертає X-координату верхнього лівого кута view-порта.
     *
     * @return uint16_t
     */
    uint16_t getViewX() const;

    /**
     * @brief Повертає Y-координату верхнього лівого кута view-порта.
     *
     * @return uint16_t
     */
    uint16_t getViewY() const;

    /**
     * @brief Малює частину ігрового рівня з указаних координат, якщо можливо.
     * Інакше малює частину ігрового рівня найближчу до цих координат.
     *
     */
    void onDraw() const;

    /**
     * @brief Встановлює вказівник на фонове зображення ігрового рівня.
     * Дані за вказівником не будуть видалені автоматично разом з ігровим рівнем.
     *
     * @param img_ptr Вказівник на фонове зображення.
     * @param img_width Ширина зображення.
     * @param img_height Висота зображення.
     * @param back_color Колір фону навколо зображення.
     * @param x_offset Зміщення зліва.
     * @param y_offset Зміщення зверху.
     */
    void setBackImg(const uint16_t* img_ptr, uint16_t img_width, uint16_t img_height, uint16_t back_color = COLOR_BLACK, uint16_t x_offset = 0, uint16_t y_offset = 0);

    /**
     * @brief Додає опис окремої плитки.
     * Дані за вказівником sprite_src не будуть видалені автоматично.
     *
     * @param sprite_id Ідентифікатор плитки.
     * @param type Тип прохідності плитки.
     * @param sprite_src Зображення плитки.
     */
    void addTileDesc(uint16_t sprite_id, TileType type, const uint16_t* sprite_src);

    /**
     * @brief Скидає опис плиток.
     * Необхідно викликати перед додаванням нових плиток, якщо потрібно перемалювати ігровий рівень в межах однії сцени.
     *
     */
    void clearTilesDesc();

    /**
     * @brief Створює поверхню ігрового рівня на основі опису плиток та таблиці розміщення плиток.
     * Ігровий рівень повиннен бути не меншим за розмір дисплея, в іншому разі, під час малювання ігрового рівня, виникне помилка.
     * Опис плиток повинен бути доданий до виклику цього методу.
     *
     * @param tiles_w_num Кількість плиток в ширину.
     * @param tiles_h_num Кількість плиток у висоту.
     * @param tile_side_len Розмір сторони плитки. Плитки повинні мати квадратну форму.
     * @param tiles_pos_templ Вказівник на масив, що містить шаблон ігрової сцени.
     */
    void build(uint16_t tiles_w_num, uint16_t tiles_h_num, uint16_t tile_side_len, const uint16_t* tiles_pos_templ);

    /**
     * @brief Перевіряє, чи може об'єкт з вказаним спрайтом бути переміщеним в указані координати ігрового рівня.
     *
     * @param x_from Х-координата точки, звідки повинне виконуватися переміщення.
     * @param y_from Y-координата точки, звідки повинне виконуватися переміщення.
     * @param x_to Х-координата точки, куди повинне виконуватися переміщення.
     * @param y_to Y-координата точки, куди повинне виконуватися переміщення.
     * @param sprite Спрайт ігрового об'єкта.
     * @return true - Якщо об'єкт може бути переміщено в указані координатах ігрового рівня. false - інакше.
     */
    bool canPass(uint16_t x_from, uint16_t y_from, uint16_t x_to, uint16_t y_to, const SpriteDescription& sprite) const;

    /**
     * @brief Повертає тип плитки у вказаних координатах.
     *
     * @param x Координата плитки.
     * @param y Координата плитки.
     * @return TileType тип плитки у вказаних координатах. TILE_TYPE_NONE - якщо плитка за вказаними координатами відсутня.
     */
    TileType getTileType(uint16_t x, uint16_t y) const;

    /**
     * @brief Встановлює позицію камери за вказаними координатами.
     *
     * @param x Координата камери.
     * @param y Координата камери.
     */
    void setCameraPos(uint16_t x, uint16_t y);

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

  private:
    void freeMem();
    void freeTilesDescriptionData();

    uint16_t coordToTilePos(uint16_t coord) const;

  private:
    std::unordered_map<uint16_t, Tile*> _tile_descr;  // Опис плиток

    Tile*** _terrain{nullptr};           // Набір плиток ігрового рівня
    const uint16_t* _back_img{nullptr};  // Фонове зображення

  public:
    const uint16_t VIEW_W;       // Ширина view-порта
    const uint16_t VIEW_H;       // Висота view-порта
    const uint16_t HALF_VIEW_W;  // Використовується для позиціонування об'єктів
    const uint16_t HALF_VIEW_H;  // Використовується для позиціонування об'єктів

  private:
    uint16_t _back_img_w{0};
    uint16_t _back_img_h{0};
    uint16_t _back_img_x_off{0};
    uint16_t _back_img_y_off{0};
    uint16_t _back_color{0};
    uint16_t _view_x{0};      // X верхнього лівого кута view-порта
    uint16_t _view_y{0};      // Y верхнього лівого кута view-порта
    uint16_t _terrain_w{0};   // Ширина ігрового рівня в пікселях
    uint16_t _terrain_h{0};   // Висота ігрового рівня в пікселях
    uint16_t _tile_x_num{0};  // Кількість плиток мапи по горизонталі
    uint16_t _tile_y_num{0};  // Кількість плиток мапи по вертикалі

    uint8_t _tile_side_len{0};  // Розмір сторони плитки
  };
}  // namespace pixeler
