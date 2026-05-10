/**
 * @file SFX.h
 * @brief Абстракція для зберігання інформації про стан wav-доріжки
 * @details Зберігає інформацію про розмір доріжки, вказівник на її дані, інформацію про поточну позицію відтворення.
 * Може повертати поточний семпл.
 * Дозвляє підсилювати семпл на відповідне значення гучності.
 */

#pragma once
#pragma GCC optimize("O3")

#include "defines.h"

namespace pixeler
{
  class SFX
  {
  public:
    SFX(const uint8_t* data_buf, uint32_t data_size);

    void play();

    void stop();

    int16_t getNextSample();

    void setOnRepeat(bool repeate);

    bool isPlaying() const;

    void setVolume(uint8_t volume);

    uint8_t getVolume() const;

    // Встановити коефіцієнт фільтрації шуму. По замовченню встановлено 1. Без фільтрації.
    void setFiltrationLvl(uint16_t lvl);

    SFX* clone();

  private:
    SFX() : _data_buf{nullptr}, _data_size{0} {}

  private:
    const uint8_t* _data_buf{nullptr};

    const uint32_t _data_size;
    uint32_t _current_sample{0};

    uint16_t _filtration_lvl{1};
    uint16_t _cached_threshold = 0;
    uint8_t _volume;
    bool _is_playing{true};
    bool _on_repeate{false};
  };
}  // namespace pixeler
