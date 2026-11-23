/**
 * @file TextBox.h
 * @brief Віджет для відображення та редагування тексту
 * @details Успадкований від Label з приховуванням частини його методів,
 * та перевизначеним малюванням віджета.
 * Підтримує два режима відображення тексту - поле для звичайного тексту та поле для вводу пароля.
 * Підтримує додавання та видалення одного символу з кінця, а також hint-підказку.
 */

#pragma once
#pragma GCC optimize("O3")

#include "../IWidget.h"
#include "Label.h"

namespace pixeler
{
  class TextBox : public Label
  {
  public:
    enum FieldType : uint8_t
    {
      TYPE_TEXT = 0,
      TYPE_PASSWORD
    };

    explicit TextBox(uint16_t widget_ID, TypeID type_ID = TYPE_TEXTBOX);
    virtual ~TextBox() {}

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
     * @return TextBox*
     */
    virtual TextBox* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_TEXTBOX;
    }

    /**
     * @brief Встановлює тип текстового поля.
     *
     * @param type Може мати значення:
     * TYPE_TEXT - текст відображається регулярними символами.
     * TYPE_PASSWORD - усі символи замінюються на *.
     */
    void setType(FieldType type);

    /**
     * @brief Повертає тип текстового поля.
     *
     * @return FieldType
     */
    FieldType getType() const;

    /**
     * @brief Встановлює текст підказки (hint), яка відображається коли поле порожнє.
     * Якщо передати порожній рядок або nullptr, використовується підказка за замовчуванням відповідно до типу поля.
     *
     * @param hint Нуль-термінований рядок UTF-8.
     */
    void setHint(const char* hint);

    /**
     * @brief Встановлює текст підказки (hint), яка відображається коли поле порожнє.
     * Якщо передати порожній рядок, використовується підказка за замовчуванням відповідно до типу поля.
     *
     * @param hint Рядок UTF-8.
     */
    void setHint(const String& hint);

    /**
     * @brief Повертає заданий користувачем текст підказки (порожній рядок, якщо використовується підказка за замовчуванням).
     *
     * @return const String&
     */
    const String& getHint() const;

    /**
     * @brief Встановлює колір тексту підказки (hint).
     *
     * @param color Колір у форматі RGB565.
     */
    void setHintColor(uint16_t color);

    /**
     * @brief Повертає колір тексту підказки (hint).
     *
     * @return uint16_t
     */
    uint16_t getHintColor() const;

    /**
     * @brief Вмикає або вимикає відображення підказки (hint) у порожньому полі.
     *
     * @param state true - показувати підказку, false - повністю приховати.
     */
    void setHintEnabled(bool state);

    /**
     * @brief Повертає стан відображення підказки (hint).
     *
     * @return true - Якщо підказку увімкнено.
     * @return false - Інакше.
     */
    bool isHintEnabled() const;

    /**
     * @brief Видаляє останній символ з рядка, що знаходиться в текстовому полі.
     *
     * @return true
     * @return false
     */
    bool removeLastChar();

    /**
     * @brief Додає послідовність символів в кінець рядка текстового поля.
     *
     * @param ch
     */
    void addChars(const char* ch);

    /**
     * @brief Додає символ в кінець рядка текстового поля.
     *
     * @param ch
     */
    void addChar(char ch);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    using Label::getAutoscrollDelay;
    using Label::hasAutoscroll;
    using Label::hasAutoscrollInFocus;
    using Label::initWidthToFit;
    using Label::setAutoscroll;
    using Label::setAutoscrollDelay;
    using Label::setAutoscrollInFocus;
    using Label::setGravity;
    using Label::setMultiline;
    using Label::updateWidthToFit;

    uint16_t getFitStr(String& ret_str) const;
    String getSubStr(const String& str, uint16_t start, uint16_t length) const;
    void drawHint(uint16_t abs_x, uint16_t abs_y, uint16_t txt_y_pos) const;

  private:
    String _hint;
    uint16_t _hint_color{COLOR_LIGHTGREY};
    FieldType _type{TYPE_TEXT};
    bool _is_hint_enabled{true};
  };
}  // namespace pixeler
