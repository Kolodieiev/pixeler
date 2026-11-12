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

    void setOnRepeat(bool repeat);
    bool isPlaying() const;

    void setVolume(uint8_t volume);
    uint8_t getVolume() const;

    // Встановити коефіцієнт фільтрації шуму. По замовчуванню встановлено 1 (без фільтрації).
    void setFiltrationLvl(uint16_t lvl);

    SFX* clone() const;

  private:
    SFX() : _data_buf{nullptr}, _data_size{0} {}
    void updateThreshold();

  private:
    const uint8_t* _data_buf{nullptr};
    const uint32_t _data_size;
    uint32_t _current_sample{0};

    uint16_t _filtration_lvl{1};
    uint16_t _cached_threshold{0};
    uint16_t _volume_scale{128};  // Масштаб 0..256 (128 = 50% за замовчуванням)
    uint8_t _volume_percent{50};  // Гучність у відсотках (0..100)
    bool _is_playing{true};
    bool _on_repeat{false};
  };
}  // namespace pixeler
