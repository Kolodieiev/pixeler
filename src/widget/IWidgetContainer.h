/**
 * @file IWidgetContainer.h
 * @brief Головний абстрактий клас, від якого повинні бути успадковані всі класи контейнерів віджетів
 * @details Містить базовий функціонал та поля, що є спільними для всіх контейнерів віджетів.
 * Керує життєвим циклом віджета після отримання вказівника на нього.
 * Слідкує за унікальністю ідентифікаторів віджетів в поточному контейнері.
 * Дозволяє виконувати пошук та видалення віджетів по їх ідентифікатору або порядковому номеру в контейнері.
 * IWidgetContainer також являється віджетом.
 */

#pragma once
#pragma GCC optimize("O3")
#include <vector>

#include "IWidget.h"

namespace pixeler
{
  class IWidgetContainer : public IWidget
  {
  public:
    IWidgetContainer(uint16_t widget_ID, TypeID type_ID);
    virtual ~IWidgetContainer();

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
     * @brief Додає вказівник на віджет до контейнера.
     * Віджет повинен мати уникальний ідентифікатор для цього контейнера.
     * Ідентифікатор віджета повинен бути більшим за 0.
     *
     * @param widget_ptr Вказівник на віджет.
     */
    void addWidget(IWidget* widget_ptr);

    /**
     * @brief Видаляє віджет з контейнера і пам'яті за його ідентифікатором.
     *
     * @param widget_ID Ідентифікатор віджета.
     * @return true - у разі успіху операції.
     * @return false - інакше.
     */
    bool delWidgetByID(uint16_t widget_ID);

    /**
     * @brief Шукає у своєму контейнері віджет з указаним ідентифікатором.
     *
     * @param widget_ID Ідентифікатор віджета.
     * @return IWidget* - Вказівник на віджет у разі успіху операції.
     * @return nullptr - Інкаше.
     */
    IWidget* getWidgetByID(uint16_t widget_ID);

    /**
     * @brief Повертає вказівник на віджет за його порядковим номером у контейнері.
     *
     * @param widget_indx Порядковий номер віджета у контейнері.
     * @return IWidget* - Вказівник на віджет.
     * @return nullptr - Якщо віджет за вказаною позицією відсутній.
     */
    IWidget* getWidgetByIndx(uint16_t widget_indx);

    /**
     * @brief Видаляє усі віджети з контейнера та очищує пам'ять, яку вони займали.
     *
     */
    void delWidgets();

    /**
     * @brief Повертає кількість віджетів у контейнері.
     *
     * @return uint16_t
     */
    uint16_t getSize() const;

    /**
     * @brief Вмикає перерисовку контейнера та його вмісту.
     *
     */
    void enable();

    /**
     * @brief Вимикає перерисовку контейнера та його вмісту.
     *
     */
    void disable();

    /**
     * @brief Повертає поточний стан прапору, який відповідає за перерисовку контейнера та його вмісту.
     *
     * @return true - Якщо перерисовка увімкнена.
     * @return false - Інакше.
     */
    bool isEnabled() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;
    
  protected:
    std::vector<IWidget*> _widgets;
    bool _is_enabled{true};
  };
}  // namespace pixeler