/**
 * @file MenuItem.h
 * @brief Базовий віджет для відображення одного елемнта меню
 * @details MenuItem містить в собі Image та віджета Label.
 */

#pragma once
#pragma GCC optimize("O3")

#include "../../image/Image.h"
#include "../../text/Label.h"

namespace pixeler
{
  // cppcheck-suppress duplInheritedMember
  class MenuItem : public IWidget
  {
  public:
    explicit MenuItem(uint16_t widget_ID, TypeID type_ID = TYPE_MENU_ITEM);
    virtual ~MenuItem() override;

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
     * @return MenuItem*
     */
    MenuItem* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_MENU_ITEM;
    }

    /**
     * @brief Встановлює вказівник на віджет Image, що буде використано в якості іконки.
     * Image буде видалено автоматично разом з віджетом.
     * Для кожного елемента списку повинен використовуватися власний Image.
     *
     * @param img Вказівник на Image.
     */
    void setImg(Image* img_ptr);

    /**
     * @brief Повертає вказівник на Image, який присвоєно цьому елементу списку.
     *
     * @return Image* - Вказівник на віджет.
     * @return nullptr - Якщо вказівник не було встановлено раніше.
     */
    Image* getImg() const;

    /**
     * @brief Встановлює вказівник на віджет Label, що буде використаний для відображення тексту елемента списку.
     * Label буде видалено автоматично разом з віджетом.
     * Для кожного елемента списку повинен використовуватися власний Label.
     *
     * @param lbl Вказівник на Label.
     */
    void setLbl(Label* lbl_ptr);

    /**
     * @brief Повертає вказівник на Label, який присвоєно цьому елементу списку.
     *
     * @return Label* - Вказівник на віджет.
     * @return nullptr - Якщо вказівник не було встановлено раніше.
     */
    Label* getLbl() const;

    /**
     * @brief Встановлює текст, що буде відображатися у віджеті.
     *
     * @param text
     */
    void setText(const String& text);

    /**
     * @brief Повертає копію тексту, що міститься в цьому елементі списку.
     * Викликає перезавантаження МК, якщо цьому елементу раніше не було присвоєно текстову мітку.
     *
     * @return String
     */
    String getText() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;
    
  protected:
    using IWidget::setVisibility;

  protected:
    Image* _img{nullptr};
    Label* _label{nullptr};

    const uint8_t ITEM_PADDING{5};
  };
}  // namespace pixeler
