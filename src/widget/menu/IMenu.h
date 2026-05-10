/**
 * @file IMenu.h
 * @brief Головний абстракний клас, від якого повинні бути успадковані всі віджети меню
 * @details Керує позиціонуванням, приховуванням та відображенням елементів меню.
 *
 * Успадкований від IWidgetContainer.
 */

#pragma once
#pragma GCC optimize("O3")

#include "../IWidgetContainer.h"
#include "item/MenuItem.h"

namespace pixeler
{
  class IMenu : public IWidgetContainer
  {
  public:
    IMenu(uint16_t widget_ID, TypeID type_ID);
    virtual ~IMenu() {}

    virtual bool focusUp() = 0;
    virtual bool focusDown() = 0;
    virtual bool pageUp() = 0;
    virtual bool pageDown() = 0;

    /**
     * @brief Повертає вказівник на самий верхній віджет, який підтримує дотики та пересікається з вказаною точкою.
     *
     * @param x - Координата точки.
     * @param y - Координата точки.
     * @return IWidget* - Вказівник на вкладений віджет або на самого себе, якщо виявлено пересічення з точкою.
     * @return nullptr - Інакше.
     */
    virtual IWidget* findTouchableAt(uint16_t x, uint16_t y) override;

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено, він автоматично пропустить перемальовування.
     *
     */
    virtual void onDraw() override;

    /**
     * @brief Видаляє всі елементи списку меню.
     *
     */
    void delWidgets();

    /**
     * @brief Видаляє елемент списку з меню і пам'яті за його ідентифікатором.
     *
     * @param widget_ID Ідентифікатор елемента списку.
     * @return true - у разі успіху операції.
     * @return false - інакше.
     */
    bool delWidgetByID(uint16_t widget_ID);

    /**
     * @brief Встановлює висоту кожного із елементів списку для вертикальної орієнтації меню.
     *
     * @param height Висота кожного елемента списку.
     */
    void setItemHeight(uint16_t height);

    /**
     * @brief Повертає висоту кожного елемента списку.
     *
     * @return uint16_t
     */
    uint16_t getItemHeight() const;

    /**
     * @brief Встановлює ширину кожного із елементів списку для горизонтальної орієнтації меню.
     *
     * @param width
     */
    void setItemWidth(uint16_t width);

    /**
     * @brief Повертає ширину кожного елемента списку.
     *
     * @return uint16_t
     */
    uint16_t getItemWidth() const;

    /**
     * @brief Встановлює орієнтацію меню.
     * При вертикальній орієнтації елементи меню розташовуються зверху вниз.
     * При горизонтальній - зліва на право.
     *
     * @param orientation Може мати значення: VERTICAL / HORIZONTAL.
     */
    void setOrientation(const Orientation orientation);

    /**
     * @brief Повертає поточну орієнтацію меню.
     *
     * @return Orientation
     */
    Orientation getOrientation() const;

    /**
     * @brief Повертає ідентифікатор віджета елемента списку, на якому встановлено фокус.
     *
     * @return uint16_t - Ідентифікатор віджета. 0 - Якщо контейнер порожній.
     */
    uint16_t getCurrItemID() const;

    /**
     * @brief Повертає порядковий номер віжета, на якому встановлено фокус, в контейнері.
     *
     * @return uint16_t - Позиція віджета в контейнері. 0 - Якщо контейнер порожній.
     */
    uint16_t getCurrFocusPos() const;

    /**
     * @brief Повертає копію тексту, що міститься у віджеті елемента списку, на якому встановлено фокус.
     *
     * @return String
     */
    String getCurrItemText() const;

    /**
     * @brief Повертає вказівник на віджет елемента списку, на якому встановлено фокус.
     *
     * @return IWidget* - Вказівник на віджет елемента списку.
     * @return nullptr - Якщо контейнер порожній.
     */
    IWidget* getCurrItem();

    /**
     * @brief Встановлює відступи між елементами меню.
     *
     * @param items_spacing Розмір відступу у пікселях між елементами списку.
     */
    void setItemsSpacing(uint16_t items_spacing);

    /**
     * @brief Повертає розмір відступу між елементами списку.
     *
     * @return uint16_t
     */
    uint16_t getItemsSpacing() const;

    /**
     * @brief Додає вказівник на віджет елемента списку до контейнера меню.
     * Віджет повинен мати уникальний ідентифікатор для цього контейнера.
     * Ідентифікатор віджета повинен бути більшим за 0.
     *
     * @param item Вказівник на віджет елемента списку.
     */
    void addItem(MenuItem* item_ptr);

    /**
     * @brief Розраховує та повертає кількість елементів, що може відобразити меню одночасно за поточних розмір.
     *
     * @return uint16_t
     */
    uint16_t getItemsPerPage() const;

    /**
     * @brief Встановлює прапор підтримки сенсорного екрану.
     *
     * @param state true - Меню не встановлює фокус для елементів списку,
     * а також вимикає підтримку усіх операцій пов'язаних з фокусом елемента.
     * @param state false - Меню встановлює фокус для елементів списку (за замовченням).
     */
    void setTouchSupport(bool state);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

    void drawItems(uint16_t start, uint16_t count);
    uint16_t getCyclesCount() const;

    using IWidgetContainer::addWidget;

  private:
    using IWidgetContainer::delWidgetByID;
    using IWidgetContainer::delWidgets;

  protected:
    uint16_t _first_item_index{0};
    uint16_t _cur_focus_pos{0};
    uint16_t _item_height{2};
    uint16_t _item_width{2};
    uint16_t _items_spacing{0};

    Orientation _orientation{VERTICAL};

    bool _has_touch_support{false};
  };
}  // namespace pixeler
