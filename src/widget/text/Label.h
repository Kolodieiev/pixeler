/**
 * @file Label.h
 * @brief Віджет для відображення тексту
 * @details Підтримує налаштування кольорів, прозорості фону, шрифту, розмірів.
 * Підтримує позиціонування тексту відносно віджета.
 * Підтримує два типи прокручування тексту, якщо текст не вміщується по ширині.
 * Підтримує автоматичне перенесення рядків.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidget.h"
#include "../image/Image.h"

namespace pixeler
{
  class Label : public IWidget
  {
  public:
    explicit Label(uint16_t widget_ID, TypeID type_ID = TYPE_LABEL);
    virtual ~Label() {};

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
     * @return Label*
     */
    virtual Label* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_LABEL;
    }

    /**
     * @brief Автоматично розраховує початкову ширину віджета.
     * Викликайте цей метод тільки після того, як налаштували інші параметри для віджета.
     * Задає початкову ширину віджета таким чином, щоб вмістити текст + відступ.
     *
     * @param add_width_value Значення, що буде додане до ширини віджета.
     */
    void initWidthToFit(uint16_t add_width_value = 0);

    /**
     * @brief Автоматично розраховує та оновлює ширину віджета таким чином, щоб вмістити текст + відступи.
     * Викликайте цей метод тільки після того, як налаштували інші відповідні параметри віджета.
     * Допомогає коректно замальовувати попереднє місце свого розрташування.
     *
     * @param add_width_value Значення, що буде додане до ширини віджета.
     */
    void updateWidthToFit(uint16_t add_width_value = 0);

    /**
     * @brief Встановлює текст, що буде відображатися у віджеті.
     *
     * @param text
     */
    void setText(const String& text);

    /**
     * @brief Повертає текст, що зараз встановлено у віджеті.
     *
     * @return String
     */
    String getText() const;

    /**
     * @brief Встановлює розмір тексту.
     *
     * @param size Розмір тексту. Значення може бути від 1 до 7.
     */
    void setTextSize(uint8_t size);

    /**
     * @brief Встановлює колір тексту.
     *
     * @param textColor
     */
    void setTextColor(uint16_t textColor);

    /**
     * @brief Встановлює вказівник на шрифт.
     *
     * @param font_ptr Вказівник на існуючий шрифт.
     */
    void setFont(const uint8_t* font_ptr);

    /**
     * @brief Повертає вказівник на поточний шрифт.
     *
     * @return uint8_t
     */
    const uint8_t* getFont() const;

    /**
     * @brief Встановлює розташування тексту по вертикалі відносно віджета.
     *
     * @param gravity Може мати значення: GRAVITY_TOP / GRAVITY_CENTER / GRAVITY_BOTTOM.
     */
    void setGravity(Gravity gravity);

    /**
     * @brief Встановлює розташування тексту по горизонталі відносно віджета.
     *
     * @param alignment Може мати значення: ALIGN_START / ALIGN_CENTER / ALIGN_END.
     */
    void setAlign(Alignment alignment);

    /**
     * @brief Встановлює горизонтальні відступи для тексту (в пікселях).
     *
     * @param padding Значення відступу.
     */
    void setHPadding(uint8_t padding);

    /**
     * @brief Повертає кількість символів, що зберігається в данному віджеті.
     * Враховуються тільки символи в кодуванні UTF-8, а не байти.
     *
     * @return uint16_t
     */
    uint16_t getLen() const;

    /**
     * @brief Встановлює прапор, який керує механізмом прокручування тексту,
     * якщо ширини віджета не достатньо для його відображення.
     *
     * @param state Якщо true - текст буде прокручуватися автоматично.
     */
    void setAutoscroll(bool state);

    /**
     * @brief Повертає значення прапору, який керує механізмом прокручування тексту.
     *
     * @return true - Текст буде прокручуватися, якщо ширина віджета недостатньо, щоб вмістити його повністю.
     * @return false - Текст не буде прокручуватися.
     */
    bool hasAutoscroll() const;

    /**
     * @brief Встановлює прапор, який керує механізмом прокручування тексту коли віджет потрапляє у фокус,
     * при цьому його ширини не достатньо, щоб повністю відобразити текст.
     *
     * @param state Якщо true - текст буде прокручуватися при потраплянні віджета у фокус.
     */
    void setAutoscrollInFocus(bool state);

    /**
     * @brief Повертає значення пропора, який керує механізмом прокручування тексту коли віджет потрапляє у фокус,
     * при цьому його ширини не достатньо, щоб повністю відобразити текст.
     *
     * @return true - Текст буде прокручуватися при потраплянні віджета у фокус.
     * @return false - Інакше.
     */
    bool hasAutoscrollInFocus() const;

    /**
     * @brief Повертає висоту шрифта цього віджета в пікселях.
     *
     * @return uint16_t
     */
    uint16_t getCharHgt() const;

    /**
     * @brief Встановлює вказівник на повністю ініціалізований віджет зображення,
     * яке буде відображатися в якості фону текстової мітки.
     * Віджет зображення не буде видалено автоматично разом із текстовою міткою.
     *
     * @param back_img Вказівник на повністю ініціалізоване зображення.
     */
    void setBackImg(Image* back_img);

    /**
     * @brief Повертає вказівник на фонове зображення цього віджета.
     *
     * @return Image*
     */
    Image* getBackImg() const;

    /**
     * @brief Встановлює стан прапора, який керує механізмом відображення тексту в декілька рядків.
     *
     * @param state
     * true - Текст буде відображатися в декільки рядків, ігноруючи стан прапора hasAutoscroll.
     * false - Текст буде відображатися в один рядок.
     */
    void setMultiline(bool state);

    /**
     * @brief Повертає стан прапора, який керує механізмом відображення тексту в декілька рядків.
     *
     * @return true - Текст може відображатися в декілька рядків, якщо не вміщується в один.
     * @return false - Текст відображається тільки в один рядок.
     */
    bool isMultiline() const;

    /**
     * @brief Встановлює затримку між зміщеннями тексту у віджеті.
     *
     * @param delay Час затримки у мілісекундах.
     */
    void setAutoscrollDelay(uint16_t delay);

    /**
     * @brief Повертає значення затримки між зміщеннями тексту у віджеті.
     *
     * @return uint16_t - Час затримки у мілісекундах.
     */
    uint16_t getAutoscrollDelay() const;

    /**
     * @brief Встановлює тип прокручування тексту.
     *
     * @param state Якщо передано true - текст у віджеті буде прокручуватися до останнього символу.
     * Інакше на 3/4 від ширини віджета.
     */
    void setFullAutoscroll(bool state);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

    uint16_t calcXStrOffset(uint16_t str_pix_num) const;
    uint16_t calcYStrOffset() const;
    uint32_t calcRealStrLen(const String& str) const;
    uint16_t calcTextPixels(uint16_t char_pos = 0) const;
    uint16_t getFitStr(String& ret_str, uint16_t start_pos = 0) const;
    String getSubStr(const String& str, uint16_t start, uint16_t length) const;
    uint32_t utf8ToUnicode(const uint8_t* buf, uint16_t& byte_pos, uint16_t remaining) const;
    uint16_t charPosToByte(const uint8_t* buf, uint16_t char_pos) const;
    void updateHeight();

  protected:
    String _text;

    unsigned long _last_autoscroll_ts{0};
    Image* _back_img{nullptr};
    const uint8_t* _font_ptr{font_unifont};

    uint16_t _text_color{COLOR_WHITE};
    uint16_t _text_len{0};
    uint16_t _autoscroll_update_delay{200};
    uint16_t _temp_width{0};
    uint16_t _first_draw_char_pos{0};
    uint16_t _char_hgt{0};
    int16_t _y_char_offset{0};

    uint8_t _text_size{1};
    uint8_t _h_padding{0};
    //
    Gravity _text_gravity{GRAVITY_TOP};
    Alignment _text_alignment{ALIGN_START};

    bool _is_multiline{false};
    bool _has_autoscroll{false};
    bool _temp_has_autoscroll{false};
    bool _has_autoscroll_in_focus{false};
    bool _temp_has_autoscroll_in_focus{false};

    bool _has_full_autoscroll{true};
    bool _is_reverse_autoscroll{false};
  };
}  // namespace pixeler
