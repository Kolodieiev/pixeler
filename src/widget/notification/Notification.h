/**
 * @file Notification.h
 * @brief Віджет для відображення багаторядкового повідомлення
 * @details Підтримує налаштування кольорів та тексту кнопок вибору, якщо потрібно.
 * Підтримує автоматичне перенесення тексту на новий рядок.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidget.h"
#include "../text/Label.h"

namespace pixeler
{
  class Notification final : public IWidget
  {
  public:
    explicit Notification(uint16_t widget_ID);
    virtual ~Notification();
    Notification(const Notification& other) = delete;
    Notification& operator=(const Notification& other) = delete;
    Notification(Notification&& other) = delete;
    Notification& operator=(Notification&& other) = delete;

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено, він автоматично пропустить перемальовування.
     *
     */
    virtual void onDraw() override;

    /**
     * @brief STUB! Не викликай!
     */
    Notification* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_NOTIFICATION;
    }

    /**
     * @brief Встановлює текст заголовка повідомлення.
     *
     * @param str
     */
    void setTitleText(const String& str);

    /**
     * @brief Встановлює основний текст повідомлення.
     *
     * @param str
     */
    void setMsgText(const String& str);

    /**
     * @brief Встановлює текст лівої кнопки повідомлення.
     *
     * @param str
     */
    void setLeftText(const String& str);

    /**
     * @brief Встановлює текст правої кнопки повідомлення.
     *
     * @param str
     */
    void setRightText(const String& str);

    /**
     * @brief Повертає вказівник на віджет заголовка повідомлення.
     *
     * @return Label*
     */
    Label* getTitleLbl();

    /**
     * @brief Повертає вказівник на віджет освного тексту повідомлення.
     *
     * @return Label*
     */
    Label* getMsgLbl();

    /**
     * @brief Повертає вказівник на віджет лівої кнопки повідомлення.
     *
     * @return Label*
     */
    Label* getLeftLbl();

    /**
     * @brief Повертає вказівник на віджет правої кнопки повідомлення.
     *
     * @return Label*
     */
    Label* getRightLbl();

  private:
    using IWidget::getFocusBackColor;
    using IWidget::removeFocus;
    using IWidget::setBorder;
    using IWidget::setCornerRadius;
    using IWidget::setFocus;
    using IWidget::setFocusBackColor;
    using IWidget::setVisibility;

    Label* _title_lbl{nullptr};
    Label* _msg_lbl{nullptr};
    Label* _left_lbl{nullptr};
    Label* _right_lbl{nullptr};
  };
}  // namespace pixeler
