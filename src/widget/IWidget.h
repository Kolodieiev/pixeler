/**
 * @file IWidget.h
 * @brief Головний абстрактий клас, від якого повинні бути успадковані всі класи віджетів
 * @details Містить базовий функціонал та поля, що є спільними для всіх віджетів.
 */

#pragma once
#pragma GCC optimize("O3")

#include "../defines.h"
#include "../driver/graphics/DisplayWrapper.h"

namespace pixeler
{
  class IWidget
  {
  public:
    enum TypeID : uint8_t
    {
      TYPE_IMAGE = 0,
      TYPE_KEYBOARD,
      TYPE_KB_ROW,
      TYPE_EMPTY_LAYOUT,
      TYPE_MENU_ITEM,
      TYPE_TOGGLE_ITEM,
      TYPE_SPIN_ITEM,
      TYPE_COMBO_ITEM,
      TYPE_DYN_MENU,
      TYPE_FIX_MENU,
      TYPE_NOTIFICATION,
      TYPE_PROGRESSBAR,
      TYPE_SCROLLBAR,
      TYPE_SPINBOX,
      TYPE_LABEL,
      TYPE_TEXTBOX,
      TYPE_TOGGLE_SWITCH,
    };

    enum Alignment : uint8_t
    {
      ALIGN_START = 0,
      ALIGN_CENTER,
      ALIGN_END
    };

    enum Gravity : uint8_t
    {
      GRAVITY_TOP = 0,
      GRAVITY_CENTER,
      GRAVITY_BOTTOM
    };

    enum Orientation : uint8_t
    {
      HORIZONTAL = 0,
      VERTICAL,
    };

    enum Visibility : uint8_t
    {
      VISIBLE = 0,
      INVISIBLE,
    };

    IWidget(uint16_t widget_ID, TypeID type_ID, bool is_container = false);
    virtual ~IWidget() = 0;

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return IWidget*
     */
    virtual IWidget* clone(uint16_t id) const = 0;

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено, він автоматично пропустить перемальовування.
     *
     */
    virtual void onDraw() = 0;

    /**
     * @brief Повертає ідентифікатор типу віджета.
     *
     * @return TypeID
     */
    TypeID getTypeID() const;

    /**
     * @brief Якщо можливо, приводить об'єкт до відповідного типу.
     * Є сенс викликати для *IWidget.
     *
     * @tparam T Тип, до якого потрібно привести віджет.
     * @return T* - Вказівник на віджет, якщо приведення можливе.
     * @return nullptr - Інакше.
     */
    template <typename T>
    T* castTo();

    /**
     * @brief Викликає примусове перемальовування віджета,
     * навіть, якщо його не було змінено.
     *
     */
    void drawForced();

    /**
     * @brief Встановлює позицію віджета відносно лівого верхнього кута свого батьківського контейнера.
     * Або відносно лівого верхнього кута дисплея, якщо батьківський контейнер відсутній.
     *
     * @param x Координата.
     * @param y Координата.
     */
    void setPos(uint16_t x, uint16_t y);

    /**
     * @brief Встановлює висоту віджета.
     *
     * @param height
     */
    void setHeight(uint16_t height);

    /**
     * @brief Встановлює ширину віджета.
     *
     * @param width
     */
    void setWidth(uint16_t width);

    /**
     * @brief Встановлює колір фону віджета.
     *
     * @param back_color
     */
    void setBackColor(uint16_t back_color);

    /**
     * @brief Встановлює вказівник на батьківський контейнер.
     * Не потрібно вказувати батьківський контейнер вручну.
     * Його буде задано під час додавання віджета до контейнера.
     *
     * @param parent
     */
    void setParent(IWidget* parent);

    /**
     * @brief Повертає вказівник на батьківський контейнер.
     *
     * @return const IWidget*
     */
    const IWidget* getParent() const;

    /**
     * @brief Встановлює значення скруглення кутів віджета.
     *
     * @param radius Значення не повинне перевищувати половину довжини коротшої сторони віджета.
     */
    void setCornerRadius(uint8_t radius);

    /**
     * @brief Встановлює стан прапору, який керує механізмом відображення межі віджету.
     * Товщина межі становить завжди 1 піксель.
     * Межа буде відображатися за рахунок ширини віджета.
     *
     * @param state Якщо true - межа віджета буде відображатися.
     */
    void setBorder(bool state);

    /**
     * @brief Встановлює колір межі віджета.
     *
     * @param color
     */
    void setBorderColor(uint16_t color);

    /**
     * @brief Повертає X координату віджета відносно верхнього лівого кута дисплея.
     *
     * @return uint16_t
     */
    uint16_t getXPos() const;

    /**
     * @brief  Повертає Y координату віджета відносно верхнього лівого кута дисплея.
     *
     * @return uint16_t
     */
    uint16_t getYPos() const;

    /**
     * @brief Повертає локальну X координату віджета.
     *
     * @return uint16_t
     */
    uint16_t getXPosLoc() const;

    /**
     * @brief Повертає локальну Y координату віджета.
     *
     * @return uint16_t
     */
    uint16_t getYPosLoc() const;

    /**
     * @brief Повертає значення скруглення кутів віджета.
     *
     * @return uint8_t
     */
    uint8_t getCornerRadius() const;

    /**
     * @brief Повертає ідентифікатор віджета, який йому було присвоєно під час створення.
     *
     * @return uint16_t
     */
    uint16_t getID() const;

    /**
     * @brief Повертає висоту віджета у пікселях.
     *
     * @return uint16_t
     */
    uint16_t getHeight() const;

    /**
     * @brief Повертає ширину віджета у пікселях.
     *
     * @return uint16_t
     */
    uint16_t getWidth() const;

    /**
     * @brief Повертає колір фону віджета.
     *
     * @return uint16_t
     */
    uint16_t getBackColor() const;

    /**
     * @brief Повертає колір межі віджета.
     *
     * @return uint16_t
     */
    uint16_t getBorderColor() const;

    /**
     * @brief Повертає стан пропору, який керує маханізмом відображення межі віджета.
     *
     * @return true - Якщо межа віджета відображається.
     * @return false - Інакше.
     */
    bool hasBorder() const;

    /**
     * @brief Встановлює стан прапору, який керує відображенням межі віджета та її кольором, під час потрапляння фокусу на нього.
     *
     * @param state Якщо true - межа буде відображатися в заданому кольорі під час отримання фокусу віджетом.
     * Попередній стан межі ігнорується, та буде відновлено після видалення фокусу з віджета.
     */
    void setChangingBorder(bool state);

    /**
     * @brief Встановлює стан прапору, який керує зміною кольору фону під час отримання фокусу віджетом.
     *
     * @param state Якщо true - фоновий колір віджета буде змінюватися відповідно до налаштувань.
     * Попереднє значення кольору фону буде автоматично відновлено після видалення фокусу з віджета.
     */
    void setChangingBack(bool state);

    /**
     * @brief Встановлює колір межі віджета у фокусі.
     *
     * @param color
     */
    void setFocusBorderColor(uint16_t color);

    /**
     * @brief Повертає значення кольору межі віджета у фокусі.
     *
     * @return uint16_t
     */
    uint16_t getFocusBorderColor() const;

    /**
     * @brief Встановлює колір фону віджета у фокусі.
     *
     * @param color
     */
    void setFocusBackColor(uint16_t color);

    /**
     * @brief Повертає колір фону віджета у фокусі.
     *
     * @return uint16_t
     */
    uint16_t getFocusBackColor() const;

    /*!
     * @brief Переводить віджет до стану "У фокусі".
     */
    void setFocus();

    /*!
     * @brief Видаляє стан "У фокусі" з віджета.
     */
    void removeFocus();

    /**
     * @brief Встановлює видимість віджета. Якщо віджет переведено в невидимий стан, він не видаляється,
     * але пропускає своє малювання або замальовує своє місце розташування фоновим кольором, якщо викликано примусове малювання.
     *
     * @param value Може мати значення: VISIBLE / INVISIBLE
     */
    void setVisibility(Visibility value);

    /**
     * @brief Повертає поточний стан видимості віджета.
     *
     * @return Visibility
     */
    Visibility getVisibility() const;

    /**
     * @brief Встановлює прозорість фону віджета.
     *
     * @param state Якщо true - фон віджета не замальовується фоновим кольором.
     * Для коректного відображення необхідно самостійно перемальовувати зображення під прозорим віджетом.
     */
    void setTransparency(bool state);

    /**
     * @brief Повертає стан прапору, який вказує, чи є віджет прозорим.
     *
     * @return true - Якщо віджет є прозорим.
     * @return false - Інакше.
     */
    bool isTransparent() const;

    /**
     * @brief Повертає вказівник на самий верхній віджет, який підтримує дотики на сенсорному екрані
     * та пересікається з вказаною точкою.
     *
     * @param x - Координата точки.
     * @param y - Координата точки.
     * @return IWidget* - Вказівник на вкладений віджет або на самого себе, якщо виявлено пересічення з точкою.
     * @return nullptr - Інакше.
     */
    virtual IWidget* findTouchableAt(uint16_t x, uint16_t y);

    /**
     * @brief Перевіряє чи пересікається віджет з точкою на дисплеї.
     *
     * @param x Координата.
     * @param y Координата.
     * @return true - Якщо віджет пересікається з вказаною точкою.
     * @return false - Інакше.
     */
    bool hitTest(uint16_t x, uint16_t y) const;

    /**
     * @brief Встановлює прапор підтримки дотика на сенсорному екрані для віджета.
     *
     * @param state
     */
    void setTouchable(bool state);

    /**
     * @brief Повертає прапор підтримки дотика на сенсорному екрані для віджета.
     *
     * @return true - Якщо віджет підтримує дотик.
     * @return false - Інакше.
     */
    bool isTouchable() const;

  protected:
    /**
     * @brief  Залити місце розташування віджета фоновим кольором та відобразити межу віджета, якщо потрібно.
     *
     * @param keep_border Прапор, який вказує, чи буде відмальовано межу.
     * True - межу буде відмальовано, якщо вона є у віджета.
     * False - малювання межі буде пропущено в будь-якому разі.
     */
    void clear(bool keep_border = true);

    /*!
     * @brief  Приховати елемент. Працює, якщо віджет має батьківський контейнер.
     */
    void hide();

    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const;

  protected:
    const IWidget* _parent{nullptr};

    uint16_t _x_pos{0};
    uint16_t _y_pos{0};
    uint16_t _width{1};
    uint16_t _height{1};
    uint16_t _back_color{COLOR_BLACK};
    uint16_t _border_color{COLOR_BLACK};
    uint16_t _focus_border_color{COLOR_WHITE};
    uint16_t _old_border_color{COLOR_WHITE};
    uint16_t _focus_back_color{COLOR_WHITE};
    uint16_t _old_back_color{COLOR_WHITE};

  private:
    const uint16_t _id;

  protected:
    Visibility _visibility{VISIBLE};
    uint8_t _corner_radius{0};

    bool _is_touchable{false};
    bool _is_changed{true};
    bool _has_border{false};
    bool _is_transparent{false};
    bool _has_focus{false};
    bool _old_border_state{false};
    bool _need_clear_border{false};
    bool _need_change_border{false};
    bool _need_change_back{false};

  private:
    const TypeID _type_ID;
  };

  /**
   * @brief Якщо можливо, приводить об'єкт до відповідного типу.
   * Є сенс викликати для *IWidget.
   *
   * @tparam T Тип, до якого потрібно привести віджет.
   * @return T* - Вказівник на віджет, якщо приведення можливе.
   * @return nullptr - Інакше.
   */
  template <typename T>
  inline T* IWidget::castTo()
  {
    if (_type_ID == T::getTypeID())
      return static_cast<T*>(this);
    else
    {
      log_e("Некоректне приведення типу %u до %u", T::getTypeID(), _type_ID);
      return nullptr;
    }
  }
}  // namespace pixeler
