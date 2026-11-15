#pragma once
#pragma GCC optimize("O3")

#include "../IWidget.h"

namespace pixeler
{
  class Image final : public IWidget
  {
  public:
    explicit Image(uint16_t widget_ID);
    virtual ~Image();

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено і не встановлено анімацію, він автоматично пропустить перемальовування.
     *
     */
    virtual void onDraw() override;

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return Image*
     */
    virtual Image* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_IMAGE;
    }

    /**
     * @brief Встановлює вказівник на буфер з даними статичного зображення.
     * Скидає поточну анімацію, якщо вона була встановлена.
     * Буфер не буде видалено разом із віджетом.
     *
     * @param image_data Вказівник на буфер з даними зображення.
     */
    void setSrc(const uint16_t* image_data);

    /**
     * @brief Встановлює вказівник на вектор кадрів анімації.
     * Скидає вказівник на статичне зображення, якщо воно було встановлене.
     * Вектор та буфери кадрів не будуть видалені разом із віджетом.
     *
     * @param animation Вказівник на вектор вказівників на кадри анімації.
     * @param frames_between Кількість пропущених кадрів між зміною кадрів анімації.
     */
    void setAnimation(const std::vector<const uint16_t*>* animation, uint8_t frames_between = 0);

    /**
     * @brief Скидає лічильники кадрів та повертає анімацію на перший кадр.
     *
     */
    void resetAnimation();

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    void drawFrame(uint16_t x, uint16_t y, const uint16_t* frame_data);

  private:
    const std::vector<const uint16_t*>* _animation{nullptr};
    const uint16_t* _img_data{nullptr};
    uint8_t _frames_counter{0};
    uint8_t _anim_pos{0};
    uint8_t _frames_between_anim{0};
  };
}  // namespace pixeler
