#pragma GCC optimize("O3")
#include "SfxPlayer.h"

#include <cmath>
#include <limits>

#include "bus/I2S_Out_Bus.h"

namespace pixeler
{
  SfxPlayer::~SfxPlayer()
  {
    if (_task_handle)
    {
      _params.cmd = TaskParams::CMD_STOP;
      vTaskDelete(_task_handle);
    }

    for (auto mix_it = _mix.begin(), last_it = _mix.end(); mix_it != last_it; ++mix_it)
      delete mix_it->second;
  }

  uint16_t SfxPlayer::addSFX(SFX* track)
  {
    if (!track)
    {
      log_e("Track ptr не може бути null");
      esp_restart();
    }

    ++_track_id;
    _mix.insert(std::pair<uint16_t, SFX*>(_track_id, track));
    return _track_id;
  }

  void SfxPlayer::removeSFX(uint16_t id)
  {
    _mix.erase(id);
  }

  void SfxPlayer::startMix()
  {
    if (!_i2s_out.isInited())
      esp_restart();

    if (_is_playing)
      return;

    _is_playing = true;

    xTaskCreatePinnedToCore(mixTask, "mixTask", (1024 / 2) * 10, this, 10, &_task_handle, 0);
  }

  void SfxPlayer::pauseResume()
  {
    if (_params.cmd == TaskParams::CMD_NONE)
    {
      _params.cmd = TaskParams::CMD_PAUSE;
    }
    else if (_params.cmd == TaskParams::CMD_PAUSE)
    {
      _params.cmd = TaskParams::CMD_NONE;
    }
  }

  void SfxPlayer::mixTask(void* params)
  {
    int16_t _samples_buf[256];

    SfxPlayer* self = static_cast<SfxPlayer*>(params);

    int16_t sample;
    uint32_t cycles_counter = 0;

    while (self->_params.cmd != TaskParams::CMD_STOP)
    {
      if (self->_params.cmd != TaskParams::CMD_PAUSE)
      {
        for (uint32_t i{0}; i < 256u; i += 2u)
        {
          sample = 0;

          for (auto it = self->_mix.begin(), last_it = self->_mix.end(); it != last_it;)
          {
            if (it->second->isPlaying())
            {
              sample += it->second->getNextSample();
              ++it;
            }
            else
            {
              delete it->second;
              it = self->_mix.erase(it);
            }
          }

          _samples_buf[i] = sample;
          _samples_buf[i + 1] = sample;
        }

        _i2s_out.write(_samples_buf, sizeof(uint16_t) * 256);
      }

      ++cycles_counter;

      if (cycles_counter > 50u)
      {
        cycles_counter = 0;
        delay(1);
      }
    }

    vTaskDelete(NULL);
  }
}  // namespace pixeler
