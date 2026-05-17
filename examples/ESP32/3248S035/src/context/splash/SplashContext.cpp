#include "SplashContext.h"

#include "../WidgetCreator.h"
#include "lib/neo_pixel/Adafruit_NeoPixel.h"
#include "manager/FileManager.h"
#include "manager/SettingsManager.h"

#define OFFSET_LBL_INFO 0
#define OFFSET_LBL_RESULT 70
#define SHOW_INIT_TIME 2000

#define LED_DATA_PIN 40
#define LED_EN_PIN 41
#define LED_COUNT 3

SplashContext::SplashContext()
{
  _display.enableBackLight();
  _start_time = millis();

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  //
  uint16_t y_pos{DISPLAY_PADDING};

  // SDCARD
  if (_fs.mount())
    addLabel(OFFSET_LBL_INFO, y_pos, STR_SUCCESS, COLOR_GREEN);
  else
    addLabel(OFFSET_LBL_INFO, y_pos, STR_FAIL, COLOR_RED);

  addLabel(OFFSET_LBL_RESULT, y_pos, "Ініціалізація SD", COLOR_WHITE);

  y_pos += _display.getFontHeight(font_unifont) + 7;

  // PSRAM
  bool psram_ok = psramInit();
  if (psram_ok)
    addLabel(OFFSET_LBL_INFO, y_pos, STR_SUCCESS, COLOR_GREEN);
  else
    addLabel(OFFSET_LBL_INFO, y_pos, STR_FAIL, COLOR_RED);

  addLabel(OFFSET_LBL_RESULT, y_pos, "Ініціалізація PSRAM", COLOR_WHITE);

  pinMode(17, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);

  digitalWrite(17, 1);
  digitalWrite(4, 1);
  digitalWrite(16, 1);
}

bool SplashContext::loop()
{
  return true;
}

void SplashContext::update()
{
  if (millis() - _start_time > SHOW_INIT_TIME)
  {
    openContextByID(ContextID::ID_CONTEXT_HOME);
  }
}

void SplashContext::addLabel(uint16_t x_pos, uint16_t y_pos, const char* str, uint16_t color)
{
  EmptyLayout* layout = getLayout()->castTo<EmptyLayout>();

  Label* lbl = new Label(_widget_id);
  layout->addWidget(lbl);
  lbl->setText(str);
  lbl->setBackColor(COLOR_MAIN_BACK);
  lbl->setTextColor(color);
  lbl->setPos(x_pos, y_pos);
  lbl->initWidthToFit();

  ++_widget_id;
}
