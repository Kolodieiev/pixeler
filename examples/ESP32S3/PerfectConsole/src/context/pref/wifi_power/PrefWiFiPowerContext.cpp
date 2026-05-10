#include "PrefWiFiPowerContext.h"

#include "../../WidgetCreator.h"
#include "manager/SettingsManager.h"
#include "manager/WiFiManager.h"

const char STR_WIFI_POWER_MIN[] = "Мінімальна";
const char STR_WIFI_POWER_MID[] = "Середня";
const char STR_WIFI_POWER_MAX[] = "Максимальна";

PrefWiFiPowerContext::PrefWiFiPowerContext()
{
  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  showMainTmpl();
}

void PrefWiFiPowerContext::showSDErrTmpl()
{
  _mode = MODE_SD_UNCONN;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  layout->addWidget(WidgetCreator::getStatusMsgLable(ID_ERR_LBL, STR_SD_ERR));
}

void PrefWiFiPowerContext::showMainTmpl()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  layout->addWidget(WidgetCreator::getWindowHeader(ID_HEADER, STR_WIFI_POWER));
  //
  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MENU_ITEM);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - 2);
  _menu->setHeight(UI_HEIGHT);
  _menu->setItemHeight((_menu->getHeight() - 2) / 5);
  //
  MenuItem* min_item = WidgetCreator::getMenuItem(ITEM_ID_MIN);
  _menu->addItem(min_item);
  Label* min_lbl = WidgetCreator::getItemLabel(STR_WIFI_POWER_MIN, font_10x20);
  min_item->setLbl(min_lbl);
  //
  MenuItem* mid_item = WidgetCreator::getMenuItem(ITEM_ID_MID);
  _menu->addItem(mid_item);
  Label* mid_lbl = WidgetCreator::getItemLabel(STR_WIFI_POWER_MID, font_10x20);
  mid_item->setLbl(mid_lbl);
  //
  MenuItem* max_item = WidgetCreator::getMenuItem(ITEM_ID_MAX);
  _menu->addItem(max_item);
  Label* max_lbl = WidgetCreator::getItemLabel(STR_WIFI_POWER_MAX, font_10x20);
  max_item->setLbl(max_lbl);
}

bool PrefWiFiPowerContext::loop()
{
  return true;
}

void PrefWiFiPowerContext::update()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);

    uint16_t power = _menu->getCurrItemID() - 1;  // Одразу переводимо в потужність з item_id
    _wifi.setPower(static_cast<WiFiManager::WiFiPowerLevel>(power));

    SettingsManager::set(STR_PREF_WIFI_POWER, String(power).c_str());

    showToast(STR_SUCCESS);
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    release();
  }
  else if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);
    _menu->focusUp();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
    _menu->focusDown();
  }
}
