/**
 * @file EmptyLayout.h
 * @brief Віджет-контейнер для об'єднання віджетів у групу без правил вирівнювання
 * @details Базова реалізація контейнера віджетів без правил вирівнювання.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidgetContainer.h"

namespace pixeler
{
  class EmptyLayout : public IWidgetContainer
  {
  public:
    explicit EmptyLayout(uint16_t widget_ID, TypeID type_ID = TYPE_EMPTY_LAYOUT);
    virtual ~EmptyLayout();

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено, він автоматично пропустить перемальовування.
     *
     */
    virtual void onDraw() override;

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return EmptyLayout*
     */
    EmptyLayout* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_EMPTY_LAYOUT;
    }

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;
  };
}  // namespace pixeler
