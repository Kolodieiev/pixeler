/**
 * @file ToggleSwitch.h
 * @brief Віджет для відображення перемикача стану
 * @details Дозволяє перемикати булеве значення між станами
 * увімкнено/вимкнено з візуальною індикацією поточного стану.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidget.h"

namespace pixeler
{
  class ToggleSwitch final : public IWidget
  {
  public:
    explicit ToggleSwitch(uint16_t widget_ID) : IWidget(widget_ID, TYPE_TOGGLE_SWITCH) {}
    virtual ~ToggleSwitch() {};

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
     * @return ToggleSwitch*
     */
    virtual ToggleSwitch* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID();

    /**
     * @brief  Повертає поточний стан перемикача.
     *
     * @return true - Якщо перемикач знаходиться в положенні "Увімкнутий".
     * @return false - Якщо перемикач знаходиться в положонні "Вимкнутий".
     */
    bool isOn() const;

    /**
     * @brief Встановлює стан перемикача.
     *
     * @param state true - Перемикач увімкнуто. false - перемикач вимкнуто.
     */
    void setOn(bool state);

    /**
     * @brief Змінює стан перемикача на протилежний від поточного.
     *
     */
    void toggle();

    /**
     * @brief Встановлює колір важелю.
     *
     * @param color
     */
    void setLeverColor(uint16_t color);

    /**
     * @brief Повертає значення кольору важеля.
     *
     * @return uint16_t
     */
    uint16_t getLeverColor() const;

    /**
     * @brief Встановлює колір фону перемикача у стані "Увімкнений".
     *
     * @param color
     */
    void setOnColor(uint16_t color);

    /**
     * @brief Повертає значення кольору фону перемикача у стані "Увімкнений".
     *
     * @return uint16_t
     */
    uint16_t getOnColor() const;

    /**
     * @brief Встановлює колір фону перемикача у стані "Ввимкнений".
     *
     * @param color
     */
    void setOffColor(uint16_t color);

    /**
     * @brief Повертає значення кольору фону перемикача у стані "Вимкнений".
     *
     * @return uint16_t
     */
    uint16_t getOffColor() const;

    /**
     * @brief Встановлює орієнтацію відображення перемикача.
     *
     * @param orientation Може мати значення: HORIZONTAL / VERTICAL.
     */
    void setOrientation(Orientation orientation);

    /**
     * @brief Повертає значення орієнтації відображення перемикача.
     *
     * @return Orientation
     */
    Orientation getOrientation() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    using IWidget::getBackColor;
    using IWidget::isTransparent;
    using IWidget::setBackColor;
    using IWidget::setTransparency;

    uint16_t _lever_color{COLOR_WHITE};
    uint16_t _on_color{COLOR_GREEN};
    uint16_t _off_color{COLOR_RED};
    Orientation _orientation{HORIZONTAL};
    bool _is_on{false};
  };
}  // namespace pixeler