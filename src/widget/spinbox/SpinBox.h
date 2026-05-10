/**
 * @file SpinBox.h
 * @brief Віджет для відображення числа в заданому діапазоні
 * @details Дозволяє змінювати числове значення інкрементом/декрементом з автоматичним контролем меж.
 * Підтримує цілі числа та числа з плаваючою крапкою.
 * Дозволяє налаштувати мінімальне та максимальне значення
 * Підтримує встановлення кроку зміни значення.
 * Значення автоматично зациклюється при досягненні меж діапазону.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../text/Label.h"

namespace pixeler
{
  class SpinBox final : public Label
  {
  public:
    enum SpinType : uint8_t
    {
      TYPE_INT = 0,
      TYPE_FLOAT
    };

    explicit SpinBox(uint16_t widget_ID);
    virtual ~SpinBox() {}

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return SpinBox*
     */
    virtual SpinBox* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_SPINBOX;
    }

    /**
     * @brief Збільшує значення, що зберігається у віджеті на величину кроку, якщо можливо.
     * Якщо значення вже досягло заданого максимуму, відбувається перехід на мінімальне.
     *
     */
    void up();

    /**
     * @brief Зменшує значення, що зберігається у віджеті на величину кроку, якщо можливо.
     * Якщо значення вже досягло заданого мінімуму, відбувається перехід на максимальне.
     *
     */
    void down();

    /**
     * @brief Встановлює мінімальне значення, яке може бути встановлено у віджеті.
     *
     * @param min Мінімально допустиме значення.
     */
    void setMin(float min);

    /**
     * @brief Повертає мінімально допустиме значення, яке може бути встановлено у віджеті.
     *
     * @return float
     */
    float getMin() const;

    /**
     * @brief Встановлює максимальне значення, яке може бути встановлено у віджеті.
     *
     * @param max Максимально допустиме значення.
     */
    void setMax(float max);

    /**
     * @brief Повертає максимально допустиме значення, яке може бути встановлено у віджеті.
     *
     * @return float
     */
    float getMax() const;

    /**
     * @brief Встановлює поточне значення.
     *
     * @param value
     */
    void setValue(float value);

    /**
     * @brief Повертає поточне значення.
     *
     * @return float
     */
    float getValue() const;

    /**
     * @brief Встановлює тип відображення вмісту.
     * По замовченню встановлено TYPE_INT.
     * Тобто число відображається без коми.
     *
     * @param spin_type Може мати значення: TYPE_INT / TYPE_FLOAT.
     */
    void setType(SpinType spin_type);

    /**
     * @brief Повертає тип відображення вмісту.
     *
     * @return SpinType
     */
    SpinType getType() const;

    /**
     * @brief Встановлює значення кроку, на яке буде змінюватися поточне значення SpinBox-у.
     *
     * @param step
     */
    void setStep(float step);

    /**
     * @brief Повертає значення кроку, на яке буде змінюватися поточне значення SpinBox-у.
     *
     * @return float
     */
    float getStep() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    using Label::hasAutoscroll;
    using Label::hasAutoscrollInFocus;
    using Label::initWidthToFit;
    using Label::setAlign;
    using Label::setAutoscroll;
    using Label::setAutoscrollInFocus;
    using Label::setGravity;
    using Label::setMultiline;
    using Label::updateWidthToFit;

    void setSpinValToDraw();

  private:
    float _min = 0.0f;
    float _max = 0.0f;
    float _value = 0.0f;
    float _step = 0.0f;

    SpinType _spin_type = TYPE_INT;
  };
}  // namespace pixeler
