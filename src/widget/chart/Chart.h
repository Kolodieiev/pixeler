/**
 * @file Chart.h
 * @brief Універсальний віджет для відображення графіків різних типів
 * @details Підтримує лінійні, площинні, стовпчикові, точкові та ступінчасті графіки,
 * декілька серій даних у режимі кільцевого буфера, фіксоване вікно телеметрії або фіксований
 * піксельний крок, повне налаштування кольорів, шрифтів, підписів осей, сітки, легенди
 * та оптимізоване часткове перемальовування області даних.
 */
#pragma once
#pragma GCC optimize("O3")

#include <vector>

#include "../IWidget.h"

namespace pixeler
{
  class Chart : public IWidget
  {
  public:
    enum ChartType : uint8_t
    {
      TYPE_LINE = 0,
      TYPE_BAR,
      TYPE_SCATTER,
      TYPE_STEP
    };

    enum XScaleMode : uint8_t
    {
      X_SCALE_AUTO = 0,
      X_SCALE_CAPACITY,
      X_SCALE_FIT,
      X_SCALE_FIXED_STEP
    };

    explicit Chart(uint16_t widget_ID, TypeID type_ID = TYPE_CHART);
    virtual ~Chart() override = default;

    /**
     * @brief Викликає процедуру малювання графіка на дисплей.
     * Автоматично виконує повне або часткове (лише область даних) перемальовування.
     */
    virtual void onDraw() override;

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return Chart*
     */
    virtual Chart* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_CHART;
    }

    /**
     * @brief Встановлює тип графіка.
     *
     * @param type TYPE_LINE / TYPE_AREA / TYPE_BAR / TYPE_SCATTER / TYPE_STEP.
     */
    void setChartType(ChartType type);

    /**
     * @brief Повертає поточний тип графіка.
     *
     * @return ChartType
     */
    ChartType getChartType() const;

    /**
     * @brief Встановлює режим розподілу точок по осі X.
     *
     * @param mode X_SCALE_AUTO (TYPE_BAR -> FIT, інші -> CAPACITY),
     * X_SCALE_CAPACITY (фіксоване вікно за місткістю буфера),
     * X_SCALE_FIT (розтягування наявних точок на всю ширину),
     * X_SCALE_FIXED_STEP (фіксований крок у пікселях).
     */
    void setXScaleMode(XScaleMode mode);

    /**
     * @brief Повертає поточний режим розподілу точок по осі X.
     *
     * @return XScaleMode
     */
    XScaleMode getXScaleMode() const;

    /**
     * @brief Встановлює фіксований горизонтальний крок між точками в пікселях.
     * При step_px > 0 автоматично перемикає режим на X_SCALE_FIXED_STEP.
     * При step_px == 0 повертає режим X_SCALE_AUTO.
     *
     * @param step_px Крок у пікселях.
     */
    void setPointStepPx(uint16_t step_px);

    /**
     * @brief Повертає встановлений фіксований крок у пікселях.
     *
     * @return uint16_t
     */
    uint16_t getPointStepPx() const;

    /**
     * @brief Встановлює ціну одного кроку між точками в одиницях осі X.
     * У режимі X_SCALE_AUTO (за замовчуванням), якщо також задано setXRange(min, max),
     * точки розташовуються з точним кроком step у межах [min, max] незалежно від розміру буфера.
     *
     * @param step Значення кроку (> 1e-6f). Передача 0 повертає автоматичний режим.
     */
    void setXStep(float step);

    /**
     * @brief Додає нову серію даних до графіка.
     *
     * @param name Назва серії для легенди.
     * @param color Основний колір серії в форматі RGB565.
     * @param max_points Максимальна кількість точок у кільцевому буфері серії.
     * @return int16_t Індекс створеної серії або -1 у разі помилки.
     */
    int16_t addSeries(const String& name, uint16_t color, uint16_t max_points = 64u);

    /**
     * @brief Видаляє серію даних за індексом.
     *
     * @param series_idx Індекс серії.
     */
    void removeSeries(uint16_t series_idx);

    /**
     * @brief Очищує всі точки вказаної серії, зберігаючи її налаштування.
     *
     * @param series_idx Індекс серії.
     */
    void clearSeriesData(uint16_t series_idx);

    /**
     * @brief Видаляє всі серії даних із графіка.
     */
    void clearAllSeries();

    /**
     * @brief Повертає кількість доданих серій даних.
     *
     * @return uint16_t
     */
    uint16_t getSeriesCount() const;

    /**
     * @brief Додає нове значення в кінець кільцевого буфера серії.
     * При досягненні max_points найстаріша точка перезаписується.
     *
     * @param series_idx Індекс серії.
     * @param value Значення точки (NaN створює розрив у лінії).
     */
    void pushPoint(uint16_t series_idx, float value);

    /**
     * @brief Повністю замінює набір точок вказаної серії.
     *
     * @param series_idx Індекс серії.
     * @param values Вказівник на масив значень.
     * @param count Кількість елементів у масиві (0 очищує серію).
     */
    void setSeriesData(uint16_t series_idx, const float* values, uint16_t count);

    /**
     * @brief Встановлює основний колір серії та автоматично оновлює колір заливки.
     *
     * @param series_idx Індекс серії.
     * @param color Колір RGB565.
     */
    void setSeriesColor(uint16_t series_idx, uint16_t color);

    /**
     * @brief Змінює назву серії у легенді.
     *
     * @param series_idx Індекс серії.
     * @param name Нова назва.
     */
    void setSeriesName(uint16_t series_idx, const String& name);

    /**
     * @brief Керує видимістю окремої серії на графіку.
     *
     * @param series_idx Індекс серії.
     * @param visible Якщо false - серія приховується з малювання та автомасштабування.
     */
    void setSeriesVisible(uint16_t series_idx, bool visible);

    /**
     * @brief Встановлює заголовок графіка.
     *
     * @param title Текст заголовка (порожній рядок приховує заголовок).
     */
    void setTitle(const String& title);

    /**
     * @brief Встановлює назву осі X.
     *
     * @param name Текст назви.
     */
    void setXAxisName(const String& name);

    /**
     * @brief Встановлює назву осі Y (відображається зверху над віссю Y).
     *
     * @param name Текст назви.
     */
    void setYAxisName(const String& name);

    /**
     * @brief Встановлює суфікс одиниць вимірювання для підписів осі X.
     *
     * @param unit Наприклад "с" або "хв".
     */
    void setXUnit(const String& unit);

    /**
     * @brief Встановлює суфікс одиниць вимірювання для підписів осі Y.
     *
     * @param unit Наприклад "°C", "V" або "%".
     */
    void setYUnit(const String& unit);

    /**
     * @brief Встановлює текстові категорійні підписи для осі X.
     * Якщо список порожній, використовуються числові підписи.
     *
     * @param labels Вектор підписів.
     */
    void setXLabels(const std::vector<String>& labels);

    /**
     * @brief Очищує текстові підписи осі X та повертає числову шкалу.
     */
    void clearXLabels();

    /**
     * @brief Задає ручний діапазон значень осі Y та вимикає автомасштабування.
     *
     * @param min_val Нижня межа.
     * @param max_val Верхня межа.
     */
    void setYRange(float min_val, float max_val);

    /**
     * @brief Задає ручний числовий діапазон підписів осі X.
     * Якщо min_val >= max_val, шкала X розраховується автоматично з урахуванням XStep.
     *
     * @param min_val Початкове значення.
     * @param max_val Кінцеве значення.
     */
    void setXRange(float min_val, float max_val);

    /**
     * @brief Вмикає або вимикає автоматичне масштабування осі Y за видимими даними.
     *
     * @param state Якщо true - межі Y обчислюються автоматично з «гарним» кроком.
     */
    void setAutoScaleY(bool state);

    /**
     * @brief Повертає стан автомасштабування осі Y.
     *
     * @return true - Якщо автомасштабування увімкнено.
     */
    bool isAutoScaleY() const;

    /**
     * @brief Встановлює кількість знаків після коми для числових підписів осей.
     *
     * @param x_decimals Знаків після коми по X (0..4).
     * @param y_decimals Знаків після коми по Y (0..4).
     */
    void setAxisDecimals(uint8_t x_decimals, uint8_t y_decimals);

    /**
     * @brief Встановлює шрифт для підписів осей, поділок та легенди.
     *
     * @param font Вказівник на шрифт (nullptr повертає font_unifont).
     */
    void setFont(const uint8_t* font);

    /**
     * @brief Встановлює множник розміру шрифту для осей та легенди.
     *
     * @param size Розмір від 1 до 7.
     */
    void setTextSize(uint8_t size);

    /**
     * @brief Встановлює шрифт для заголовка графіка.
     *
     * @param font Вказівник на шрифт (nullptr повертає font_unifont).
     */
    void setTitleFont(const uint8_t* font);

    /**
     * @brief Встановлює множник розміру шрифту для заголовка.
     *
     * @param size Розмір від 1 до 7.
     */
    void setTitleSize(uint8_t size);

    /**
     * @brief Встановлює колір фону внутрішньої області побудови графіка.
     *
     * @param color Колір RGB565.
     */
    void setPlotBackColor(uint16_t color);

    /**
     * @brief Встановлює колір ліній осей X та Y.
     *
     * @param color Колір RGB565.
     */
    void setAxisColor(uint16_t color);

    /**
     * @brief Встановлює колір ліній координатної сітки.
     *
     * @param color Колір RGB565.
     */
    void setGridColor(uint16_t color);

    /**
     * @brief Встановлює колір опорної лінії нуля (Y = 0).
     *
     * @param color Колір RGB565.
     */
    void setZeroLineColor(uint16_t color);

    /**
     * @brief Встановлює колір тексту заголовка.
     *
     * @param color Колір RGB565.
     */
    void setTitleColor(uint16_t color);

    /**
     * @brief Встановлює колір підписів осей та числових поділок.
     *
     * @param color Колір RGB565.
     */
    void setTextColor(uint16_t color);

    /**
     * @brief Встановлює колір тексту легенди.
     *
     * @param color Колір RGB565.
     */
    void setLegendTextColor(uint16_t color);

    /**
     * @brief Керує відображенням координатної сітки.
     *
     * @param state Якщо true - сітка відображається.
     */
    void setShowGrid(bool state);

    /**
     * @brief Встановлює пунктирний або суцільний стиль ліній сітки.
     *
     * @param state Якщо true - лінії сітки пунктирні.
     */
    void setGridDashed(bool state);

    /**
     * @brief Задає кількість інтервалів сітки та поділок по осях X і Y.
     *
     * @param x_divs Кількість інтервалів по X (мінімум 1).
     * @param y_divs Кількість інтервалів по Y (мінімум 1).
     */
    void setGridDivisions(uint8_t x_divs, uint8_t y_divs);

    /**
     * @brief Керує відображенням ліній осей X та Y.
     *
     * @param state Якщо true - осі відображаються.
     */
    void setShowAxes(bool state);

    /**
     * @brief Керує відображенням підписів поділок уздовж осі X.
     *
     * @param state Якщо true - підписи X відображаються.
     */
    void setShowXLabels(bool state);

    /**
     * @brief Керує відображенням підписів поділок уздовж осі Y.
     *
     * @param state Якщо true - підписи Y відображаються.
     */
    void setShowYLabels(bool state);

    /**
     * @brief Керує відображенням легенди серій.
     *
     * @param state Якщо true - легенда відображається.
     */
    void setShowLegend(bool state);

    /**
     * @brief Керує відображенням маркерів у вузлових точках (для TYPE_LINE / TYPE_AREA / TYPE_STEP).
     *
     * @param state Якщо true - малюються точкові маркери.
     */
    void setShowMarkers(bool state);

    /**
     * @brief Встановлює радіус маркерів точок у пікселях.
     *
     * @param radius Радіус (1..6 пікселів).
     */
    void setMarkerRadius(uint8_t radius);

    /**
     * @brief Встановлює товщину лінії графіка в пікселях.
     *
     * @param thickness Товщина (1..3 пікселі).
     */
    void setLineThickness(uint8_t thickness);

    /**
     * @brief Керує відображенням горизонтальної лінії Y = 0, якщо нуль входить у діапазон.
     *
     * @param state Якщо true - лінія нуля відображається.
     */
    void setShowZeroLine(bool state);

    /**
     * @brief Встановлює проміжок між групами стовпчиків у режимі TYPE_BAR.
     *
     * @param gap Проміжок у пікселях.
     */
    void setBarGap(uint8_t gap);

    /**
     * @brief Встановлює внутрішній відступ від країв віджета до елементів графіка.
     *
     * @param padding Відступ у пікселях.
     */
    void setPadding(uint8_t padding);

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget Цільовий віджет.
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    struct Series
    {
      std::vector<float> data;
      String name;
      uint16_t color{COLOR_WHITE};
      uint16_t head{0u};
      uint16_t count{0u};
      uint16_t capacity{0u};
      bool visible{true};
    };

    XScaleMode getEffectiveXScaleMode() const;
    bool isPhysicalXStepActive() const;
    float getSeriesValue(const Series& series, uint16_t logical_idx) const;
    uint16_t calcMaxPoints() const;
    uint16_t calcMaxCapacity() const;
    uint16_t calcVisibleStartIdx(uint16_t point_count, int32_t pw) const;
    bool updateAutoScaleRange();
    void recalcLayout();

    void fillPlotBackground(uint16_t abs_x, uint16_t abs_y) const;
    void drawTitleAndLegend(uint16_t abs_x, uint16_t abs_y) const;
    void drawGrid(uint16_t abs_x, uint16_t abs_y) const;
    void drawZeroLine(uint16_t abs_x, uint16_t abs_y) const;
    void drawAxes(uint16_t abs_x, uint16_t abs_y) const;
    void drawAxisLabels(uint16_t abs_x, uint16_t abs_y) const;
    void drawPlotContent(uint16_t abs_x, uint16_t abs_y) const;
    void drawLineLikeSeries(const Series& series, int32_t px, int32_t py, int32_t pw, int32_t ph, uint16_t denom_pts, float inv_y_span) const;
    void drawBarSeries(int32_t px, int32_t py, int32_t pw, int32_t ph, uint16_t max_pts, uint16_t max_cap, float inv_y_span, int32_t base_y) const;

    void drawClippedLine(int32_t x0, float y0_f, int32_t x1, float y1_f, int32_t px, int32_t py, int32_t pw, int32_t ph, uint16_t color) const;
    void drawClippedMarker(int32_t cx, int32_t cy, int32_t px, int32_t py, int32_t pw, int32_t ph, uint16_t color) const;
    void drawDashedHLine(int32_t x, int32_t y, int32_t len, uint16_t color) const;
    void drawDashedVLine(int32_t x, int32_t y, int32_t len, uint16_t color) const;
    float valueToYFloat(float value, int32_t py, int32_t ph, float inv_y_span) const;
    void formatValue(char* out_buf, size_t buf_size, float value, uint8_t decimals, const String& unit) const;

  private:
    std::vector<Series> _series;
    std::vector<String> _x_labels;
    String _title;
    String _x_axis_name;
    String _y_axis_name;
    String _x_unit;
    String _y_unit;
    const uint8_t* _font{font_unifont};
    const uint8_t* _title_font{font_unifont};

    float _x_min{0.0f};
    float _x_max{0.0f};
    float _x_step{0.0f};
    float _y_min{0.0f};
    float _y_max{100.0f};
    float _eff_y_min{0.0f};
    float _eff_y_max{100.0f};

    uint16_t _plot_back_color{0x1082u};
    uint16_t _axis_color{0xAD55u};
    uint16_t _grid_color{0x3186u};
    uint16_t _zero_line_color{0x7BEFu};
    uint16_t _title_color{COLOR_WHITE};
    uint16_t _text_color{0xCE79u};
    uint16_t _legend_text_color{COLOR_WHITE};
    uint16_t _point_step_px{4u};
    uint16_t _cached_width{0u};
    uint16_t _cached_height{0u};
    uint16_t _last_max_points{0u};
    uint16_t _plot_x_loc{0u};
    uint16_t _plot_y_loc{0u};
    uint16_t _plot_w{0u};
    uint16_t _plot_h{0u};
    uint16_t _title_y_loc{0u};
    uint16_t _legend_y_loc{0u};
    uint16_t _y_name_y_loc{0u};
    uint16_t _char_hgt{0u};
    uint16_t _title_char_hgt{0u};
    int16_t _y_char_offset{0};
    int16_t _title_y_offset{0};

    ChartType _chart_type{TYPE_LINE};
    XScaleMode _x_scale_mode{X_SCALE_AUTO};
    uint8_t _text_size{1u};
    uint8_t _title_size{1u};
    uint8_t _padding{4u};
    uint8_t _grid_x_divs{4u};
    uint8_t _grid_y_divs{4u};
    uint8_t _line_thickness{1u};
    uint8_t _marker_radius{2u};
    uint8_t _bar_gap{2u};
    uint8_t _x_decimals{0u};
    uint8_t _y_decimals{1u};

    bool _auto_scale_y{true};
    bool _show_grid{true};
    bool _grid_dashed{true};
    bool _show_axes{true};
    bool _show_x_labels{true};
    bool _show_y_labels{true};
    bool _show_legend{true};
    bool _show_markers{false};
    bool _show_zero_line{true};
    bool _is_data_changed{true};
    bool _is_layout_dirty{true};
    bool _has_drawn_once{false};
    bool _cached_has_border{false};
  };
}  // namespace pixeler
