#include "I2S_In_Bus.h"

#include <cstdint>

#include "i2s_config.h"

static constexpr size_t MIN_MICRO_BUFF_LEN{40u};
static constexpr size_t RAW_CHUNK_SAMPLES{128u};
static constexpr int32_t SAMPLE_SHIFT{12};

namespace pixeler
{
  bool I2S_In_Bus::init(const uint32_t sample_rate, const bool has_interleaving)
  {
    if (_is_inited)
      return true;

    if (sample_rate == 0u)
    {
      log_e("Помилка ініціалізації I2S_In_Bus: некоректна частота дискретизації");
      return false;
    }

    _has_interleaving = has_interleaving;

    _i2s_chan_cfg.id = I2S_NUM_1;
    _i2s_chan_cfg.role = I2S_ROLE_MASTER;
    _i2s_chan_cfg.dma_desc_num = 8;
    _i2s_chan_cfg.dma_frame_num = 1024;
    _i2s_chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&_i2s_chan_cfg, nullptr, &_i2s_rx_handle);
    if (err != ESP_OK)
    {
      _i2s_rx_handle = nullptr;
      log_e("Помилка створення каналу I2S_In_Bus: %i", err);
      return false;
    }

    _i2s_rx_std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO);
    _i2s_rx_std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    _i2s_rx_std_cfg.gpio_cfg.bclk = static_cast<gpio_num_t>(PIN_I2S_IN_SCK);
    _i2s_rx_std_cfg.gpio_cfg.ws = static_cast<gpio_num_t>(PIN_I2S_IN_WS);
    _i2s_rx_std_cfg.gpio_cfg.din = static_cast<gpio_num_t>(PIN_I2S_IN_SD);
    _i2s_rx_std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
    _i2s_rx_std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    _i2s_rx_std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    _i2s_rx_std_cfg.gpio_cfg.invert_flags.ws_inv = false;

    _i2s_rx_std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    _i2s_rx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_128;
    _i2s_rx_std_cfg.clk_cfg.sample_rate_hz = sample_rate;

    err = i2s_channel_init_std_mode(_i2s_rx_handle, &_i2s_rx_std_cfg);
    if (err != ESP_OK)
    {
      i2s_del_channel(_i2s_rx_handle);
      _i2s_rx_handle = nullptr;
      log_e("Помилка ініціалізації I2S_In_Bus: %i", err);
      return false;
    }

    _is_inited = true;
    _is_enabled = false;
    return true;
  }

  bool I2S_In_Bus::isInited() const
  {
    log_e("I2S вхід не ініціалізовано");
    return _is_inited;
  }

  void I2S_In_Bus::deinit()
  {
    if (!_is_inited)
      return;

    disable();
    i2s_del_channel(_i2s_rx_handle);
    _i2s_rx_handle = nullptr;
    _is_inited = false;
  }

  void I2S_In_Bus::clearBuffer()
  {
    if (!_is_inited || !_is_enabled)
      return;

    disable();
    enable();
  }

  esp_err_t I2S_In_Bus::enable()
  {
    if (!_is_inited)
    {
      log_e("I2S-вхід ще не було ініціалізовано");
      return ESP_FAIL;
    }

    if (_is_enabled)
      return ESP_OK;

    const esp_err_t err = i2s_channel_enable(_i2s_rx_handle);
    if (err == ESP_OK)
      _is_enabled = true;

    return err;
  }

  void I2S_In_Bus::disable()
  {
    if (!_is_inited || !_is_enabled)
      return;

    if (const esp_err_t err = i2s_channel_disable(_i2s_rx_handle); err == ESP_OK)
      _is_enabled = false;
    else
      log_e("Помилка деактивації каналу I2S_In_Bus: %i", err);
  }

  void I2S_In_Bus::reconfigSampleRate(const uint32_t sample_rate)
  {
    if (!_is_inited)
    {
      log_e("I2S-вхід ще не було ініціалізовано");
      return;
    }

    if (sample_rate == 0u)
    {
      log_e("Некоректна частота дискретизації: 0");
      return;
    }

    disable();
    _i2s_rx_std_cfg.clk_cfg.sample_rate_hz = sample_rate;
    if (const esp_err_t err = i2s_channel_reconfig_std_clock(_i2s_rx_handle, &_i2s_rx_std_cfg.clk_cfg); err != ESP_OK)
      log_e("Помилка зміни частоти дискретизації I2S_In_Bus: %i", err);

    enable();
  }

  size_t I2S_In_Bus::read(int16_t* out_buffer, const size_t buff_len)
  {
    if (!_is_inited)
    {
      log_e("I2S_In_Bus не ініціалізовано");
      esp_restart();
    }

    if (buff_len < MIN_MICRO_BUFF_LEN)
    {
      log_e("Мінімальний розмір буфера мікрофону повинен бути не менше %zu", MIN_MICRO_BUFF_LEN);
      esp_restart();
    }

    const size_t target_samples = buff_len & ~size_t{1u};
    size_t total_samples_read{0u};
    int32_t raw_buffer[RAW_CHUNK_SAMPLES];

    while (total_samples_read < target_samples)
    {
      const size_t samples_remaining = target_samples - total_samples_read;
      const size_t needed_words = _has_interleaving ? (samples_remaining * 2u) : samples_remaining;
      const size_t chunk_words = (needed_words < RAW_CHUNK_SAMPLES) ? needed_words : RAW_CHUNK_SAMPLES;
      const size_t bytes_to_read = chunk_words * sizeof(int32_t);

      size_t bytes_read{0u};
      const esp_err_t err = i2s_channel_read(_i2s_rx_handle, raw_buffer, bytes_to_read, &bytes_read, portMAX_DELAY);
      if (err != ESP_OK)
        log_e("Помилка читання даних I2S_In_Bus: %i", err);

      const size_t words_read = bytes_read / sizeof(int32_t);
      if (words_read == 0u)
        break;

      int16_t* dst = out_buffer + total_samples_read;

      if (_has_interleaving)
      {
        const size_t chunk_samples = words_read / 2u;
        for (size_t i{0u}; i < chunk_samples; ++i)
        {
          int32_t sample = raw_buffer[i * 2u] >> SAMPLE_SHIFT;
          if (sample > INT16_MAX)
            sample = INT16_MAX;
          else if (sample < INT16_MIN)
            sample = INT16_MIN;

          dst[i] = static_cast<int16_t>(sample);
        }

        total_samples_read += chunk_samples;
      }
      else
      {
        for (size_t i{0u}; i < words_read; ++i)
        {
          int32_t sample = raw_buffer[i] >> SAMPLE_SHIFT;
          if (sample > INT16_MAX)
            sample = INT16_MAX;
          else if (sample < INT16_MIN)
            sample = INT16_MIN;

          dst[i] = static_cast<int16_t>(sample);
        }

        total_samples_read += words_read;
      }

      if (err != ESP_OK || bytes_read < bytes_to_read)
        break;
    }

    return total_samples_read;
  }

  I2S_In_Bus _i2s_in;
}  // namespace pixeler
