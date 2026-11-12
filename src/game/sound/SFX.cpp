#include "SFX.h"

#include <algorithm>

#define MAX_VOLUME 100

namespace pixeler
{
  SFX::SFX(const uint8_t* data_buf, uint32_t data_size)
      : _data_buf{data_buf}, _data_size{data_size}
  {
    setVolume(50);
  }

  void SFX::play()
  {
    if (_data_buf && _data_size >= 2)
      _is_playing = true;

    _current_sample = 0;
  }

  void SFX::stop()
  {
    _is_playing = false;
  }

  int16_t SFX::getNextSample()
  {
    if (!_is_playing || !_data_buf || _current_sample + 2 > _data_size)
      return 0;

    // Безпечне зчитування Little-Endian int16_t за будь-якого вирівнювання буфера
    int16_t raw_sample = static_cast<int16_t>(
        _data_buf[_current_sample] | (_data_buf[_current_sample + 1] << 8));

    int32_t sample = (static_cast<int32_t>(raw_sample) * _volume_scale) >> 8;

    if (std::abs(sample) < _cached_threshold)
      sample = 0;

    _current_sample += 2;

    if (_current_sample + 2 > _data_size)
    {
      if (!_on_repeat)
        _is_playing = false;
      _current_sample = 0;
    }

    return static_cast<int16_t>(sample);
  }

  void SFX::setOnRepeat(bool repeat)
  {
    _on_repeat = repeat;
  }

  bool SFX::isPlaying() const
  {
    return _is_playing;
  }

  void SFX::setVolume(uint8_t volume)
  {
    _volume_percent = std::min<uint8_t>(volume, MAX_VOLUME);
    _volume_scale = (static_cast<uint32_t>(_volume_percent) * 256u) / MAX_VOLUME;
    updateThreshold();
  }

  uint8_t SFX::getVolume() const
  {
    return _volume_percent;
  }

  void SFX::setFiltrationLvl(uint16_t lvl)
  {
    _filtration_lvl = lvl;
    updateThreshold();
  }

  void SFX::updateThreshold()
  {
    _cached_threshold = (_volume_scale * _filtration_lvl) >> 8;
  }

  SFX* SFX::clone() const
  {
    SFX* copy = new SFX(*this);
    if (!copy)
    {
      log_e("Помилка клонування звукової доріжки");
      esp_restart();
    }
    return copy;
  }
}  // namespace pixeler
