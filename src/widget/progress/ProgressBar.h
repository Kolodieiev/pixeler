/**
 * @file ProgressBar.h
 * @brief Віджет для відображення прогресу
 * @details Підтримує налаштування кольорів.
 * Підтримує встановлення максимального значення прогресу, відносно якого розраховується заповнення.
 * Підтримує зміну орієнтації.
 */

#pragma once
#pragma GCC optimize("O3")
#include "../IWidget.h"

namespace pixeler
{
  class ProgressBar final : public IWidget
  {
  public:
    explicit ProgressBar(uint16_t widget_ID);
    virtual ~ProgressBar();

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
     * @return ProgressBar*
     */
    virtual ProgressBar* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_PROGRESSBAR;
    }

    /**
     * @brief Встановлює максимальне значення прогресу.
     *
     * @param max
     */
    void setMax(uint32_t max);

    /**
     * @brief Встановлює поточне значення прогресу.
     *
     * @param progress
     */
    void setProgress(uint32_t progress);

    /**
     * @brief Встановлює колір фону смужки прогресу.
     *
     * @param color
     */
    void setProgressColor(uint16_t color);

    /**
     * @brief Встановлює орієнтацію віджета.
     * По замовчуванню вастановлено HORIZONTAL.
     *
     * @param orientation Може мати значення: VERTICAL / HORIZONTAL.
     */
    void setOrientation(Orientation orientation);

    /**
     * @brief Повертає значення орієнтації віджета.
     *
     * @return Orientation
     */
    Orientation getOrientation() const;

    /**
     * @brief Скидає значення прогресу до 1.
     *
     */
    void reset();

    /**
     * @brief Повертає поточне значення прогресу.
     *
     * @return uint32_t
     */
    uint32_t getProgress() const;

    /**
     * @brief Повертає прогрес віджета відносно координат.
     *
     * @param x Координата.
     * @param y Координата.
     * @return uint32_t - Прогрес відносно координат.
     * @return 0 - Якщо параметри некоректні.
     */
    uint32_t getProgressAt(uint16_t x, uint16_t y) const;

    /**
     * @brief Повертає максимально можливе значення прогресу.
     *
     * @return uint32_t
     */
    uint32_t getMax() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    using IWidget::isTransparent;
    using IWidget::setTransparency;

  private:
    uint32_t _progress{1};
    uint32_t _max{1};
    uint32_t _prev_progress{1};
    uint16_t _progress_color{COLOR_WHITE};
    //
    Orientation _orientation{HORIZONTAL};
    // opt
    bool _is_first_draw{true};
  };

}  // namespace pixeler
