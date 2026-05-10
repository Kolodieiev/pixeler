#include "Arduino_ESP32DSIPanel.h"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

Arduino_ESP32DSIPanel::Arduino_ESP32DSIPanel(
    uint16_t hsync_pulse_width,
    uint16_t hsync_back_porch,
    uint16_t hsync_front_porch,
    uint16_t vsync_pulse_width,
    uint16_t vsync_back_porch,
    uint16_t vsync_front_porch,
    uint32_t bus_freq_mhz,
    uint16_t lane_bit_rate,
    soc_periph_mipi_dsi_phy_pllref_clk_src_t clock_source)
    : _hsync_pulse_width{hsync_pulse_width},
      _hsync_back_porch{hsync_back_porch},
      _hsync_front_porch{hsync_front_porch},
      _vsync_pulse_width{vsync_pulse_width},
      _vsync_back_porch{vsync_back_porch},
      _vsync_front_porch{vsync_front_porch},
      _bus_freq_hz{static_cast<uint16_t>(bus_freq_mhz / 1000000u)},
      _lane_bit_rate{lane_bit_rate},
      _clock_source{clock_source}
{
}

bool Arduino_ESP32DSIPanel::begin(uint16_t w, uint16_t h, const lcd_init_cmd_t* init_operations, size_t init_operations_len)
{
  esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
  const esp_ldo_channel_config_t ldo_mipi_phy_config = {
      .chan_id = 3,
      .voltage_mv = 2500,
  };

  if (esp_err_t ret = esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy) != ESP_OK)
  {
    log_e("Помилка увімкнення живлення MIPI: %s", esp_err_to_name(ret));
    esp_restart();
  }

  log_i("Живлення MIPI увімкнуто");

  //----------------------------------------------------------------------------------------------

  const esp_lcd_dsi_bus_config_t bus_config = {
      .bus_id = 0,
      .num_data_lanes = 2,
      .phy_clk_src = _clock_source,
      .lane_bit_rate_mbps = _lane_bit_rate,
  };

  esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;

  if (esp_err_t ret = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus) != ESP_OK)
  {
    log_e("Помилка ініціалізації шини MIPI DSI: %s", esp_err_to_name(ret));
    esp_restart();
  }

  log_i("Шину MIPI DSI ініціалізовано");

  //----------------------------------------------------------------------------------------------
  const esp_lcd_dbi_io_config_t dbi_config = {
      .virtual_channel = 0,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };

  esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;

  if (esp_err_t ret = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io) != ESP_OK)
  {
    log_e("Помилка створення MIPI DBI: %s", esp_err_to_name(ret));
    esp_restart();
  }

  log_i("MIPI DBI інтерфейс створено");

  //----------------------------------------------------------------------------------------------

  esp_lcd_video_timing_t video_timing = {
      .h_size = w,
      .v_size = h,
      .hsync_pulse_width = _hsync_pulse_width,
      .hsync_back_porch = _hsync_back_porch,
      .hsync_front_porch = _hsync_front_porch,
      .vsync_pulse_width = _vsync_pulse_width,
      .vsync_back_porch = _vsync_back_porch,
      .vsync_front_porch = _vsync_front_porch};

  esp_lcd_dpi_panel_config_t dpi_config = {
      .virtual_channel = 0,
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = _bus_freq_hz,
      .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
      .in_color_format = LCD_COLOR_FMT_RGB565,
      .out_color_format = LCD_COLOR_FMT_RGB565,
      .num_fbs = 2,
      .video_timing = video_timing,
      .flags = {
          .use_dma2d = true,
          .disable_lp = true}};

  if (esp_err_t ret = esp_lcd_new_panel_dpi(mipi_dsi_bus, &dpi_config, &_panel_handle) != ESP_OK)
  {
    log_e("Помилка створення MIPI DPI панелі: %s", esp_err_to_name(ret));
    esp_restart();
  }

  log_i("Панель MIPI DPI створена успішно");

  //----------------------------------------------------------------------------------------------

  for (int i = 0; i < init_operations_len; i++)
  {
    if (esp_err_t ret = esp_lcd_panel_io_tx_param(mipi_dbi_io, init_operations[i].cmd, init_operations[i].data, init_operations[i].data_bytes) != ESP_OK)
    {
      log_e("Помилка надсилання команд ініціалізації: %s", esp_err_to_name(ret));
      esp_restart();
    }
    delay(init_operations[i].delay_ms);
  }
  log_i("Команди ініціалізації надіслані успішно");

  if (esp_err_t ret = esp_lcd_panel_init(_panel_handle))
  {
    log_e("Помилка ініціалізації панелі: %s", esp_err_to_name(ret));
    esp_restart();
  }
  log_i("Дисплей успішно ініціалізовано");

  return true;
}

esp_lcd_panel_handle_t Arduino_ESP32DSIPanel::getPanelHandle()
{
  return _panel_handle;
}

#endif  // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)