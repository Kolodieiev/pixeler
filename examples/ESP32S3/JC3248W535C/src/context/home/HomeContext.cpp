#include "HomeContext.h"

#include "../WidgetCreator.h"
#include "../resources/ico/battery.h"
#include "manager/SettingsManager.h"
#include "util/batt_util.h"

#define UPD_DISPLAY_INTERVAL_MS 5000UL

HomeContext::HomeContext()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  String walpp_path = SettingsManager::get(STR_WALLPP_FILENAME);

  if (!walpp_path.isEmpty())
  {
    BmpLoader loader;
    _wall_res = loader.load(walpp_path.c_str());

    if (_wall_res)
    {
      Image* wallpp_img = new Image(ID_WALLPAPER);
      layout->addWidget(wallpp_img);
      wallpp_img->setWidth(_wall_res->getWidth());
      wallpp_img->setHeight(_wall_res->getHeight());
      wallpp_img->setSrc(static_cast<const uint16_t*>(_wall_res->getData()));
    }
  }

  _batt_volt_lbl = new Label(ID_BAT_LVL);
  layout->addWidget(_batt_volt_lbl);
  _batt_volt_lbl->setText(STR_EMPTY_BAT);
  _batt_volt_lbl->setWidth(ICO_BATTERY_WIDTH);
  _batt_volt_lbl->setHeight(ICO_BATTERY_HEIGHT);
  _batt_volt_lbl->setFont(font_5x7);
  _batt_volt_lbl->setAlign(Label::ALIGN_CENTER);
  _batt_volt_lbl->setGravity(Label::GRAVITY_CENTER);
  _batt_volt_lbl->setPos(UI_WIDTH - DISPLAY_PADDING - _batt_volt_lbl->getWidth(), DISPLAY_PADDING);
  _batt_volt_lbl->setTransparency(true);

  _batt_ico = new Image(1);
  _batt_volt_lbl->setBackImg(_batt_ico);
  _batt_ico->setWidth(_batt_volt_lbl->getWidth());
  _batt_ico->setHeight(_batt_volt_lbl->getHeight());
  _batt_ico->setSrc(ICO_BATTERY);
  _batt_ico->setTransparency(true);

  updateBattVoltage();
}

HomeContext::~HomeContext()
{
  delete _batt_ico;
  delete _wall_res;
}

bool HomeContext::loop()
{
  return true;
}

void HomeContext::update()
{
  // if (_input.isPressed())
  // {
  //   _input.lock(CLICK_LOCK);
  //   log_i("isPressed");
  // }
  // else if (_input.isHolded())
  // {
  //   log_i("isHolded");
  // }
  // else

  if (_input.isReleased())
  {
    openContextByID(ID_CONTEXT_MENU);
  }

  // if (millis() - _upd_timer > UPD_DISPLAY_INTERVAL_MS)
  // {
  //   _upd_timer = millis();
  //   updateBattVoltage();
  // }
}

void HomeContext::updateBattVoltage()
{
  // float bat_voltage = readBattVoltage();
  float bat_voltage = 4.2f;
  String volt_str = String(bat_voltage);
  _batt_volt_lbl->setText(volt_str);

  getLayout()->drawForced();
}
