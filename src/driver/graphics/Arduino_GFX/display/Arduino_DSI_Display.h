#pragma once
#pragma GCC optimize("O3")

#include "../Arduino_DataBus.h"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
#include "../Arduino_GFX.h"
#include "../databus/Arduino_ESP32DSIPanel.h"

class Arduino_DSI_Display : public Arduino_GFX
{
public:
  Arduino_DSI_Display(
      Arduino_ESP32DSIPanel* dsi_panel,
      uint16_t w,
      uint16_t h,
      uint8_t rotation = 0,
      int8_t rst = GFX_NOT_DEFINED);

  virtual bool begin(int32_t speed = GFX_NOT_DEFINED) override;
  virtual void draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) override;

private:
  static bool IRAM_ATTR lcd_trans_done_cb(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t* edata, void* user_ctx);

protected:
  Arduino_ESP32DSIPanel* _dsi_panel;
  esp_lcd_panel_handle_t _panel_handle{nullptr};
  volatile SemaphoreHandle_t _dsi_semaphore{nullptr};

  void* _lcd_draw_buffers[2] = {nullptr, nullptr};
  void* _draw_buffer{nullptr};

  size_t _framebuffer_size{0};

  const uint16_t MAX_X;
  const uint16_t MAX_Y;
  uint8_t _back_fb_i{0};
  const int8_t PIN_RST;

private:
};

#endif  // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
