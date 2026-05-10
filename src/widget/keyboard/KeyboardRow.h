/**
 * @file KeyboardRow.h
 * @brief Віджет для формування та відображення одного ряду віртуальної клавіатури
 * @details Формує з Label один ряд віртуальної клавіатури ти виводить його на Canvas.
 * Керує переміщенням фокусу між окремими кнопками в рядку.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidgetContainer.h"

namespace pixeler
{
  class KeyboardRow final : public IWidgetContainer
  {
  public:
    explicit KeyboardRow(uint16_t widget_ID);
    virtual ~KeyboardRow() {}

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
     * @return KeyboardRow*
     */
    virtual KeyboardRow* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_KB_ROW;
    }

    /**
     * @brief Повертає ідентифікатор віджета, на якому встановлено фокус.
     *
     * @return uint16_t
     */
    uint16_t getCurrBtnID() const;

    /**
     * @brief Повертає копію тексту, що міститься у віджеті, на якому встановлено фокус.
     *
     * @return String
     */
    String getCurrBtnTxt() const;

    /**
     * @brief Переміщує фокус у контейнері на один віджет вгору.
     *
     * @return true - Якщо операція виконана успішно.
     * @return false - Інакше.
     */
    bool focusUp();

    /**
     * @brief Переміщує фокус у контейнері на один віджет униз.
     *
     * @return true - Якщо операція виконана успішно.
     * @return false - Інакше.
     */
    bool focusDown();

    /**
     * @brief Встановлює висоту кнопок у цьому контейнері.
     *
     * @param height Значення висоти кнопок у пікселях.
     */
    void setBtnHeight(uint16_t height);

    /**
     * @brief Повертає значення висоти кнопок у пікселях.
     *
     * @return uint16_t
     */
    uint16_t getBtnsHeight() const;

    /**
     * @brief Встановлює ширину кнопок у цьому контейнері.
     *
     * @param width Значення ширини кнопок у пікселях.
     */
    void setBtnWidth(uint16_t width);

    /**
     * @brief Повертає значення ширини кнопок у пікселях.
     *
     * @return uint16_t
     */
    uint16_t getBtnsWidth() const;

    /**
     * @brief Повертає позицію кнопки для данного ряду, на якій встановлено фокус.
     *
     * @return uint16_t
     */
    uint16_t getCurFocusPos() const;

    /**
     * @brief Встановлює фокус на віджеті з указаним порядковим номером.
     *
     * @param pos Порядковий номер віджета кнопки у контейнері.
     */
    void setFocus(uint16_t pos);

    /**
     * @brief Видаляє фокус з віджета в даному контейнері.
     *
     */
    void removeFocus();

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    IWidget* getFocusBtn() const;

  private:
    uint16_t _cur_focus_pos{0};
    uint16_t _btn_height{1};
    uint16_t _btn_width{1};
  };
}  // namespace pixeler
