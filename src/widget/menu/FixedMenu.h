/**
 * @file FixedMenu.h
 * @brief Віджет меню, що дозволяє відобразити фіксований, визначений раніше список елементів
 * @details Керує переміщенням фокусу по елементах меню.
 * Підтримує зациклювання меню.
 *
 * Успадкований від IMenu.
 */

#pragma once
#pragma GCC optimize("O3")
#include "IMenu.h"

namespace pixeler
{
  class FixedMenu final : public IMenu
  {
  public:
    explicit FixedMenu(uint16_t widget_ID);
    virtual ~FixedMenu() {}

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return FixedMenu*
     */
    virtual FixedMenu* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_FIX_MENU;
    }

    /**
     * @brief Рендерить попередню сторінку меню.
     *
     * @return true - Якщо операцію виконано успішно.
     * @return false - Інакше.
     */
    virtual bool pageUp() override;

    /**
     * @brief Рендерить наступну сторінку меню.
     *
     * @return true - Якщо операцію виконано успішно.
     * @return false - Інакше.
     */
    virtual bool pageDown() override;

    /**
     * @brief Переміщує фокус на попередній віджет у контейнері.
     *
     * @return true - Якщо операцію виконано успішно.
     * @return false - Інакше.
     */
    virtual bool focusUp() override;

    /**
     * @brief Переміщує фокус на наступний віджет у контейнері.
     *
     * @return true - Якщо операцію виконано успішно.
     * @return false - Інакше.
     */
    virtual bool focusDown() override;

    /**
     * @brief Встановлює стан прапору, який відповідає за механізм зациклювання меню.
     *
     * @param state Якщо true - меню буде зациклене.
     * Якщо false - фокус не буде переходити на протилежний край списку при досягненні його кінця.
     */
    void setLoopState(bool state);

    /**
     * @brief Встановлює фокус в списку на віджеті за його порядковим номером у контейнері.
     * Якщо елемент на вказаній позиції відсутній, поточний фокус не буде змінено.
     *
     * @param focus_pos Порядковий номер віджета в контейнері.
     */
    void setCurrFocusPos(uint16_t focus_pos);

    /**
     * @brief Повертає кількість сторінок, які містить меню з поточною кількістю елементів.
     * 
     * @return uint16_t 
     */
    uint16_t getPagesCount() const;

    /**
     * @brief Повертає номер поточної сторінки меню.
     * 
     * @return uint16_t 
     */
    uint16_t getPageNum() const;

    /**
     * @brief Встановлює номер поточної сторінки меню.
     * 
     * @param page_pos 
     */
    void setPageNum(uint16_t page_pos);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  protected:
    bool _is_loop_enbl{false};
  };
}  // namespace pixeler
