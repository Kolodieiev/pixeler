/**
 * @file DynamicMenu.h
 * @brief Віджет меню, що дозволяє димнамічно завантажувати та відображати елементи меню
 * @details Керує переміщенням фокусу по елементах меню.
 * Під час досягення одного з кінців списку, намагається завантажити наступну частину
 * меню через зворотній виклик відповідної функції-завантажувача.
 * Рекомендований для відображення довгих меню, які займають великий об'єм оперативної пам'яті.
 *
 * Успадкований від IMenu.
 */

#pragma once
#pragma GCC optimize("O3")

#include "IMenu.h"

namespace pixeler
{
  class DynamicMenu final : public IMenu
  {
  public:
    /**
     * @brief Тип функції-обробника, яку може бути викликано для завантаження наступної сторінки динамічного меню.
     *
     */
    typedef std::function<void(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)> NextItemsLoadHandler_t;

    /**
     * @brief Тип функції-обробника, яку може бути викликано для завантаження попередньої сторінки динамічного меню.
     *
     */
    typedef std::function<void(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)> PrevItemsLoadHandler_t;

    explicit DynamicMenu(uint16_t widget_ID);
    virtual ~DynamicMenu() {}

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return DynamicMenu*
     */
    virtual DynamicMenu* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_DYN_MENU;
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
     * @brief Встановлює обробник, який буде викликано для завантаження наступної частини віджетів для меню.
     *
     * @param handler Обробник.
     * @param arg Аргумент, що буде передано в обробник.
     */
    void setOnNextItemsLoadHandler(NextItemsLoadHandler_t handler, void* arg);

    /**
     * @brief Встановлює обробник, який буде викликано для завантаження попередньої частини віджетів для меню.
     *
     * @param handler Обробник.
     * @param arg Аргумент, що буде передано в обробник.
     */
    void setOnPrevItemsLoadHandler(PrevItemsLoadHandler_t handler, void* arg);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
  private:
    NextItemsLoadHandler_t _next_items_load_handler{nullptr};
    PrevItemsLoadHandler_t _prev_items_load_handler{nullptr};

    void* _next_items_load_arg{nullptr};
    void* _prev_items_load_arg{nullptr};
  };

}  // namespace pixeler
