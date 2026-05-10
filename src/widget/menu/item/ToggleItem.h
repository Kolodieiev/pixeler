/**
 * @file ToggleItem.h
 * @brief Віджет елемента меню з ToggleSwitch
 * @details Дозволяє перемикати булеве значення (увімкнено/вимкнено)
 * безпосередньо в елементі меню через вбудований ToggleSwitch.
 *
 * Успадкований від MenuItem.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../../toggle/ToggleSwitch.h"
#include "MenuItem.h"

namespace pixeler
{
  // cppcheck-suppress duplInheritedMember
  class ToggleItem final : public MenuItem
  {
  public:
    explicit ToggleItem(uint16_t widget_ID);
    virtual ~ToggleItem() override;

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
     * @return ToggleItem*
     */
    ToggleItem* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_TOGGLE_ITEM;
    }

    /**
     * @brief Встановлює вказівник на віджет ToggleSwitch, що буде відображатися у віджеті елемента списку.
     * ToggleSwitch буде видалено автоматично разом з віджетом.
     * Для кожного елемента списку повинен використовуватися власний ToggleSwitch.
     *
     * @param togg_switch_ptr Вказівник на ToggleSwitch.
     */
    void setToggle(ToggleSwitch* togg_switch_ptr);

    /**
     * @brief Повертає вказівник на ToggleSwitch, який присвоєно цьому елементу списку.
     *
     * @return ToggleSwitch* - Вказівник на віджет.
     */
    ToggleSwitch* getToggle() const;

    /**
     * @brief Встановлює стан ToggleSwitch.
     *
     */
    void setOn(bool state);

    /**
     * @brief Змінює стан ToggleSwitch на протилежний від поточного.
     *
     */
    void toggle();

    /**
     * @brief Повертає значення прапору, що вказує на поточний стан ToggleSwitch.
     *
     * @return true - Якщо ToggleSwitch знаходиться у стані "Ввімкнений".
     * @return false - Якщо ToggleSwitch знаходиться у стані "Вимкнений"
     * або вказівник на ToggleSwitch не було встановлено..
     */
    bool isOn() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    ToggleSwitch* _toggle{nullptr};
  };
}  // namespace pixeler
