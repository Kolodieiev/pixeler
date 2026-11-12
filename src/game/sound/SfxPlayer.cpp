#pragma GCC optimize("O3")
#include "SfxPlayer.h"

#include <algorithm>
#include <cstring>

#include "bus/I2S_Out_Bus.h"

namespace pixeler
{
  SfxPlayer::SfxPlayer()
  {
    _mutex = xSemaphoreCreateMutex();
    _mix.reserve(10);
  }

  SfxPlayer::~SfxPlayer()
  {
    if (_is_playing)
    {
      _params.cmd = TaskParams::CMD_STOP;
      while (_is_playing)
      {
        delay(1);
      }
    }

    clear();

    if (_mutex)
      vSemaphoreDelete(_mutex);
  }

  uint16_t SfxPlayer::addSFX(SFX* track)
  {
    if (!track)
    {
      log_e("Track ptr не може бути null");
      esp_restart();
    }

    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (++_track_id == 0)
      _track_id = 1;

    uint16_t assigned_id = _track_id;
    _mix.push_back({assigned_id, track});

    xSemaphoreGive(_mutex);
    return assigned_id;
  }

  void SfxPlayer::removeSFX(uint16_t id)
  {
    xSemaphoreTake(_mutex, portMAX_DELAY);

    for (size_t i = 0; i < _mix.size(); ++i)
    {
      if (_mix[i].id == id)
      {
        delete _mix[i].sfx;
        _mix[i] = _mix.back();
        _mix.pop_back();
        break;
      }
    }

    xSemaphoreGive(_mutex);
  }

  void SfxPlayer::clear()
  {
    xSemaphoreTake(_mutex, portMAX_DELAY);

    for (auto& entry : _mix)
      delete entry.sfx;

    _mix.clear();

    xSemaphoreGive(_mutex);
  }

  void SfxPlayer::setMasterVolume(uint8_t volume)
  {
    _master_vol_percent = std::min<uint8_t>(volume, 100);
    _master_scale = (static_cast<uint32_t>(_master_vol_percent) * 256u) / 100u;
  }

  uint8_t SfxPlayer::getMasterVolume() const
  {
    return _master_vol_percent;
  }

  void SfxPlayer::startMix()
  {
    if (!_i2s_out.isInited())
      esp_restart();

    if (_is_playing)
      return;

    _params.cmd = TaskParams::CMD_NONE;
    _is_playing = true;

    xTaskCreatePinnedToCore(mixTask, "mixTask", (1024 / 2) * 10, this, 10, &_task_handle, 0);
  }

  void SfxPlayer::pauseResume()
  {
    if (_params.cmd == TaskParams::CMD_NONE)
      _params.cmd = TaskParams::CMD_PAUSE;
    else if (_params.cmd == TaskParams::CMD_PAUSE)
      _params.cmd = TaskParams::CMD_NONE;
  }

  void SfxPlayer::mixTask(void* params)
  {
    int16_t _samples_buf[256];
    SfxPlayer* self = static_cast<SfxPlayer*>(params);
    uint32_t cycles_counter = 0;

    while (self->_params.cmd != TaskParams::CMD_STOP)
    {
      if (self->_params.cmd != TaskParams::CMD_PAUSE)
      {
        xSemaphoreTake(self->_mutex, portMAX_DELAY);

        if (!self->_mix.empty())
        {
          const uint16_t master_scale = self->_master_scale;

          for (uint32_t i{0}; i < 256u; i += 2u)
          {
            int32_t mixed_sample = 0;

            for (size_t k = 0; k < self->_mix.size();)
            {
              SFX* sfx = self->_mix[k].sfx;
              if (sfx->isPlaying())
              {
                mixed_sample += sfx->getNextSample();
                ++k;
              }
              else
              {
                delete sfx;
                self->_mix[k] = self->_mix.back();
                self->_mix.pop_back();
              }
            }

            mixed_sample = (mixed_sample * master_scale) >> 8;
            int16_t final_sample = static_cast<int16_t>(
                std::clamp<int32_t>(mixed_sample, -32768, 32767));

            _samples_buf[i] = final_sample;
            _samples_buf[i + 1] = final_sample;
          }
        }
        else
        {
          // Якщо активних треків немає — заповнюємо буфер тишею за <1 мкс,
          // щоб DMA-буфер I2S був чистим і не повторював останній звук
          std::memset(_samples_buf, 0, sizeof(_samples_buf));
        }

        xSemaphoreGive(self->_mutex);

        // Запис у I2S виконується поза м'ютексом завдяки чому синхронізує потік по DMA
        _i2s_out.write(_samples_buf, sizeof(uint16_t) * 256);
      }
      else
      {
        delay(5);
      }

      if (++cycles_counter > 50u)
      {
        cycles_counter = 0;
        delay(1);
      }
    }

    self->_task_handle = nullptr;
    self->_is_playing = false;
    vTaskDelete(NULL);
  }
}  // namespace pixeler
