#include "I2S_Out_Bus.h"

#include "i2s_config.h"

static constexpr size_t MONO_CHUNK_SAMPLES{128u};
static constexpr size_t CLEAR_BUFF_SIZE{128u};

namespace pixeler
{
  bool I2S_Out_Bus::init(const uint32_t sample_rate)
  {
    if (_is_inited)
      return true;

    if (sample_rate == 0u)
    {
      log_e("Помилка ініціалізації I2S_Out_Bus: некоректна частота дискретизації");
      return false;
    }

    _i2s_chan_cfg.id = I2S_NUM_0;
    _i2s_chan_cfg.role = I2S_ROLE_MASTER;
    _i2s_chan_cfg.dma_desc_num = 8;
    _i2s_chan_cfg.dma_frame_num = 1024;
    _i2s_chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&_i2s_chan_cfg, &_i2s_tx_handle, nullptr);
    if (err != ESP_OK)
    {
      _i2s_tx_handle = nullptr;
      log_e("Помилка створення каналу I2S_Out_Bus: %i", err);
      return false;
    }

    if (!_comm_fmt)
      _i2s_tx_std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    else
      _i2s_tx_std_cfg.slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

    _i2s_tx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    _i2s_tx_std_cfg.gpio_cfg.bclk = static_cast<gpio_num_t>(PIN_I2S_OUT_BCLK);
    _i2s_tx_std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
    _i2s_tx_std_cfg.gpio_cfg.dout = static_cast<gpio_num_t>(PIN_I2S_OUT_DOUT);
    _i2s_tx_std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    _i2s_tx_std_cfg.gpio_cfg.ws = static_cast<gpio_num_t>(PIN_I2S_OUT_LRC);
    _i2s_tx_std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    _i2s_tx_std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    _i2s_tx_std_cfg.gpio_cfg.invert_flags.ws_inv = false;
    _i2s_tx_std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    _i2s_tx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_128;
    _i2s_tx_std_cfg.clk_cfg.sample_rate_hz = sample_rate;

    err = i2s_channel_init_std_mode(_i2s_tx_handle, &_i2s_tx_std_cfg);
    if (err != ESP_OK)
    {
      i2s_del_channel(_i2s_tx_handle);
      _i2s_tx_handle = nullptr;
      log_e("Помилка ініціалізації I2S_Out_Bus: %i", err);
      return false;
    }

    _is_inited = true;
    _is_enabled = false;
    clearBuffer();
    log_i("I2S-вихід успішно ініціалізовано");
    return true;
  }

  void I2S_Out_Bus::reconfigSampleRate(const uint32_t sample_rate)
  {
    if (!_is_inited)
    {
      log_e("I2S-вихід ще не було ініціалізовано");
      return;
    }

    if (sample_rate == 0u)
    {
      log_e("Некоректна частота дискретизації: 0");
      return;
    }

    disable();
    _i2s_tx_std_cfg.clk_cfg.sample_rate_hz = sample_rate;
    if (const esp_err_t err = i2s_channel_reconfig_std_clock(_i2s_tx_handle, &_i2s_tx_std_cfg.clk_cfg); err != ESP_OK)
      log_e("Помилка зміни частоти дискретизації: %i", err);

    clearBuffer();
    enable();
  }

  void I2S_Out_Bus::setI2SCommFMT_LSB(const bool comm_fmt)
  {
    _comm_fmt = comm_fmt;

    if (!_is_inited)
      return;

    const bool was_enabled = _is_enabled;
    disable();

    if (!_comm_fmt)
      _i2s_tx_std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    else
      _i2s_tx_std_cfg.slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

    _i2s_tx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    if (const esp_err_t err = i2s_channel_reconfig_std_slot(_i2s_tx_handle, &_i2s_tx_std_cfg.slot_cfg); err != ESP_OK)
      log_e("Помилка переконфігурації слоту I2S: %i", err);

    if (was_enabled)
      enable();
  }

  size_t I2S_Out_Bus::write(const int16_t* buffer, const size_t buff_len, const bool only_left_chan)
  {
    if (!_is_inited)
    {
      log_e("I2S-вихід ще не було ініціалізовано");
      esp_restart();
    }

    if (!buffer || buff_len == 0u)
      return 0u;

    if (!only_left_chan)
    {
      size_t bytes_written{0u};
      if (const esp_err_t err = i2s_channel_write(_i2s_tx_handle, buffer, buff_len * sizeof(int16_t), &bytes_written, portMAX_DELAY); err != ESP_OK)
        log_e("Помилка відтворення даних: %i", err);

      return bytes_written;
    }

    size_t total_written{0u};
    size_t offset{0u};
    int16_t stereo_chunk[MONO_CHUNK_SAMPLES * 2u];

    while (offset < buff_len)
    {
      const size_t remaining = buff_len - offset;
      const size_t chunk_len = (remaining < MONO_CHUNK_SAMPLES) ? remaining : MONO_CHUNK_SAMPLES;
      int16_t* dst = stereo_chunk;

      for (size_t i{0u}; i < chunk_len; ++i)
      {
        const int16_t sample = buffer[offset + i];
        *dst++ = sample;
        *dst++ = sample;
      }

      size_t chunk_written{0u};
      if (const esp_err_t err = i2s_channel_write(_i2s_tx_handle, stereo_chunk, chunk_len * 2u * sizeof(int16_t), &chunk_written, portMAX_DELAY); err != ESP_OK)
      {
        log_e("Помилка відтворення даних: %i", err);
        total_written += chunk_written;
        break;
      }

      total_written += chunk_written;
      offset += chunk_len;
    }

    return total_written;
  }

  bool I2S_Out_Bus::isInited() const
  {
    if (!_is_inited)
      log_e("I2S вихід не ініціалізовано");

    return _is_inited;
  }

  void I2S_Out_Bus::deinit()
  {
    if (!_is_inited)
      return;

    disable();
    i2s_del_channel(_i2s_tx_handle);
    _i2s_tx_handle = nullptr;

    pinMode(PIN_I2S_OUT_BCLK, OUTPUT);
    digitalWrite(PIN_I2S_OUT_BCLK, HIGH);

    pinMode(PIN_I2S_OUT_LRC, OUTPUT);
    digitalWrite(PIN_I2S_OUT_LRC, HIGH);

    pinMode(PIN_I2S_OUT_DOUT, OUTPUT);
    digitalWrite(PIN_I2S_OUT_DOUT, HIGH);

    _is_inited = false;
  }

  esp_err_t I2S_Out_Bus::enable()
  {
    if (!_is_inited)
    {
      log_e("I2S-вихід ще не було ініціалізовано");
      return ESP_FAIL;
    }

    if (_is_enabled)
      return ESP_OK;

    const esp_err_t err = i2s_channel_enable(_i2s_tx_handle);
    if (err == ESP_OK)
      _is_enabled = true;

    return err;
  }

  void I2S_Out_Bus::disable()
  {
    if (!_is_inited || !_is_enabled)
      return;

    if (const esp_err_t err = i2s_channel_disable(_i2s_tx_handle); err == ESP_OK)
      _is_enabled = false;
    else
      log_e("Помилка деактивації каналу I2S: %i", err);
  }

  void I2S_Out_Bus::clearBuffer()
  {
    if (!_is_inited)
      return;

    const bool was_enabled = _is_enabled;
    if (was_enabled)
      disable();

    const uint8_t buff[CLEAR_BUFF_SIZE]{};
    size_t bytes_loaded{0u};
    if (const esp_err_t err = i2s_channel_preload_data(_i2s_tx_handle, buff, CLEAR_BUFF_SIZE, &bytes_loaded); err != ESP_OK)
      log_e("Помилка очищення буфера I2S: %i", err);

    if (was_enabled)
      enable();
  }

  I2S_Out_Bus _i2s_out;
}  // namespace pixeler
