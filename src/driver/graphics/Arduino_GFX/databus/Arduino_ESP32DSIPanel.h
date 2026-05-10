#pragma once

#include "../Arduino_DataBus.h"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_ldo_regulator.h>

typedef struct
{
  uint8_t cmd;         /*<! The specific LCD command */
  const uint8_t* data; /*<! Buffer that holds the command specific data */
  uint8_t data_bytes;  /*<! Size of `data` in memory, in bytes */
  uint32_t delay_ms;   /*<! Delay in milliseconds after this command */
} lcd_init_cmd_t;

class Arduino_ESP32DSIPanel
{
public:
  Arduino_ESP32DSIPanel(
      uint16_t hsync_pulse_width,
      uint16_t hsync_back_porch,
      uint16_t hsync_front_porch,
      uint16_t vsync_pulse_width,
      uint16_t vsync_back_porch,
      uint16_t vsync_front_porch,
      uint32_t bus_freq_mhz,
      uint16_t lane_bit_rate,
      soc_periph_mipi_dsi_phy_pllref_clk_src_t clock_source);

  bool begin(uint16_t w, uint16_t h, const lcd_init_cmd_t* init_operations = NULL, size_t init_operations_len = GFX_NOT_DEFINED);

  esp_lcd_panel_handle_t getPanelHandle();

private:
  esp_lcd_panel_handle_t _panel_handle{nullptr};
  
  soc_periph_mipi_dsi_phy_pllref_clk_src_t _clock_source{MIPI_DSI_PHY_PLLREF_CLK_SRC_PLL_F20M};
  uint16_t _hsync_pulse_width;
  uint16_t _hsync_back_porch;
  uint16_t _hsync_front_porch;
  uint16_t _vsync_pulse_width;
  uint16_t _vsync_back_porch;
  uint16_t _vsync_front_porch;
  uint16_t _lane_bit_rate;
  uint16_t _bus_freq_hz;
};

#endif  // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)