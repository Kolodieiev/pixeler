#include "SplashContext.h"

#include "../WidgetCreator.h"
#include "lib/neo_pixel/Adafruit_NeoPixel.h"
#include "manager/FileManager.h"
#include "manager/SettingsManager.h"
#include "manager/WiFiManager.h"

#define OFFSET_LBL_INFO 0
#define OFFSET_LBL_RESULT 70
#define SHOW_INIT_TIME 1500

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
  uint16_t y_pos{0};

  // SDCARD
  if (_fs.mount())
  {
    addLabel(OFFSET_LBL_INFO, y_pos, STR_SUCCESS, COLOR_GREEN);

    String bright = SettingsManager::get(STR_PREF_BRIGHT);

    if (bright.equals("") || bright.equals("0"))
      _display.setBrightness(240);
    else
      _display.setBrightness(atoi(bright.c_str()));
  }
  else
  {
    addLabel(OFFSET_LBL_INFO, y_pos, STR_FAIL, COLOR_RED);
  }

  addLabel(OFFSET_LBL_RESULT, y_pos, "Ініціалізація SD", COLOR_WHITE);

  y_pos += _display.getFontHeight(font_unifont) + 7;

  // PSRAM
  bool psram_ok = psramInit();
  if (psram_ok)
    addLabel(OFFSET_LBL_INFO, y_pos, STR_SUCCESS, COLOR_GREEN);
  else
    addLabel(OFFSET_LBL_INFO, y_pos, STR_FAIL, COLOR_RED);

  addLabel(OFFSET_LBL_RESULT, y_pos, "Ініціалізація PSRAM", COLOR_WHITE);

  blinkLED();
}

bool SplashContext::loop()
{
  return true;
}

void SplashContext::update()
{
  if (millis() - _start_time > SHOW_INIT_TIME)
  {
    if (_fs.isMounted())
    {
      String wifi_autoconn = SettingsManager::get(STR_PREF_WIFI_AUTOCONNECT, STR_WIFI_SUBDIR);
      if (wifi_autoconn.equals("1"))
      {
        String last_ssid = SettingsManager::get(STR_PREF_WIFI_LAST_SSID, STR_WIFI_SUBDIR);
        if (!last_ssid.isEmpty())
        {
          String last_ssid_pwd = SettingsManager::get(last_ssid.c_str(), STR_WIFI_SUBDIR);
          _wifi.enable();
          _wifi.tryConnectTo(last_ssid, last_ssid_pwd);
        }
      }
    }

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

void SplashContext::blinkLED()
{
  String led_greet_str = SettingsManager::get(STR_PREF_LED_GREET);
  if (led_greet_str.equals("") || led_greet_str.equals("0"))
    return;

  pinMode(LED_EN_PIN, OUTPUT);
  digitalWrite(LED_EN_PIN, HIGH);

  Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
  strip.setBrightness(110);
  strip.begin();

  strip.clear();
  strip.setPixelColor(0, strip.Color(255, 0, 255));
  strip.show();
  delay(200);

  strip.clear();
  for (int i = 0; i < 2; i++)
    strip.setPixelColor(i, strip.Color(255, 112, 255));
  strip.show();
  delay(200);

  for (int i = 0; i < LED_COUNT; i++)
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  strip.show();
  delay(200);

  digitalWrite(LED_EN_PIN, LOW);
  pinMode(LED_EN_PIN, INPUT);
  pinMode(LED_DATA_PIN, INPUT);
}
