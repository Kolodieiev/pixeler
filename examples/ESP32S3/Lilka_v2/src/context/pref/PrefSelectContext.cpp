#include "PrefSelectContext.h"

#include "../WidgetCreator.h"
#include "widget/menu/item/ToggleItem.h"
#include "widget/toggle/ToggleSwitch.h"
//
#include "bright/PrefBrightContext.h"
#include "file_server/PrefFileServerContext.h"
#include "manager/SettingsManager.h"
#include "wifi_power/PrefWiFiPowerContext.h"

static const char STR_AUDIO_MONO[] = "Монозвук";
static const char STR_WIFI_AUTOCONNECT[] = "Автопідключення до WiFi";
static const char STR_AUDIO_AMP[] = "Підсилювач звуку";
static const char STR_LED_GREET[] = "Привітання LED";
static const char STR_FILE_SERVER[] = "Файловий сервер";

void PrefSelectContext::showSDErrTmpl()
{
  _mode = MODE_SD_UNCONN;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  layout->addWidget(WidgetCreator::getStatusMsgLable(ID_ERR_LBL, STR_SD_ERR));
}

PrefSelectContext::PrefSelectContext()
{
  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  showMainTmpl();
}

void PrefSelectContext::showMainTmpl()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MENU_ITEM);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - 2);
  _menu->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2);
  _menu->setItemHeight((_menu->getHeight() - 2) / 5);
  _menu->setPos(0, DISPLAY_CUTOUT);

  // Монозвук
  ToggleItem* mono_item = new ToggleItem(ITEM_ID_AUDIO_MONO);
  _menu->addItem(mono_item);
  mono_item->setFocusBorderColor(COLOR_LIME);
  mono_item->setFocusBackColor(COLOR_FOCUS_BACK);
  mono_item->setChangingBorder(true);
  mono_item->setChangingBack(true);

  Label* mono_lbl = WidgetCreator::getItemLabel(STR_AUDIO_MONO, font_10x20);
  mono_item->setLbl(mono_lbl);

  ToggleSwitch* toggle_mono = new ToggleSwitch(ID_TOGGLE);
  mono_item->setToggle(toggle_mono);
  toggle_mono->setWidth(40);
  toggle_mono->setHeight(22);
  toggle_mono->setCornerRadius(7);

  String mono_mode = SettingsManager::get(STR_PREF_MONO_AUDIO);
  if (mono_mode.equals("1"))
    toggle_mono->setOn(true);
  else
    toggle_mono->setOn(false);

  // Автоматичне підключення wifi
  ToggleItem* wifi_autoconn_item = mono_item->clone(ITEM_ID_WIFI_AUTOCONN);
  _menu->addItem(wifi_autoconn_item);
  wifi_autoconn_item->getLbl()->setText(STR_WIFI_AUTOCONNECT);

  String wifi_autoconn = SettingsManager::get(STR_PREF_WIFI_AUTOCONNECT, STR_WIFI_SUBDIR);
  if (wifi_autoconn.equals("1"))
    wifi_autoconn_item->getToggle()->setOn(true);
  else
    wifi_autoconn_item->getToggle()->setOn(false);

  //
  MenuItem* bright_item = WidgetCreator::getMenuItem(ITEM_ID_BRIGHT);
  _menu->addItem(bright_item);
  Label* bright_lbl = WidgetCreator::getItemLabel(STR_BRIGHT, font_10x20);
  bright_item->setLbl(bright_lbl);

  //

  ToggleItem* audio_amp_item = mono_item->clone(ITEM_ID_AUDIO_AMP);
  _menu->addItem(audio_amp_item);
  audio_amp_item->getLbl()->setText(STR_AUDIO_AMP);

  String audio_amp_str = SettingsManager::get(STR_PREF_AUDIO_AMP);
  if (audio_amp_str.equals("1"))
    audio_amp_item->getToggle()->setOn(true);
  else
    audio_amp_item->getToggle()->setOn(false);

  //

  ToggleItem* led_greet_item = mono_item->clone(ITEM_ID_LED_GREET);
  _menu->addItem(led_greet_item);
  led_greet_item->getLbl()->setText(STR_LED_GREET);

  String led_greet_str = SettingsManager::get(STR_PREF_LED_GREET);
  if (led_greet_str.equals("1"))
    led_greet_item->getToggle()->setOn(true);
  else
    led_greet_item->getToggle()->setOn(false);

  //

  MenuItem* file_server_item = WidgetCreator::getMenuItem(ITEM_ID_FILE_SERVER);
  _menu->addItem(file_server_item);
  Label* file_server_lbl = WidgetCreator::getItemLabel(STR_FILE_SERVER, font_10x20);
  file_server_item->setLbl(file_server_lbl);

  //

  MenuItem* wifi_power_item = WidgetCreator::getMenuItem(ITEM_ID_WIFI_POWER);
  _menu->addItem(wifi_power_item);
  Label* wifi_power_lbl = WidgetCreator::getItemLabel(STR_WIFI_POWER, font_10x20);
  wifi_power_item->setLbl(wifi_power_lbl);

  //

  _scrollbar = new ScrollBar(ID_SCROLLBAR);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2);
  _scrollbar->setPos(UI_WIDTH - SCROLLBAR_WIDTH, DISPLAY_CUTOUT);
  _scrollbar->setMax(_menu->getSize());
}

bool PrefSelectContext::loop()
{
  if (_mode == MODE_SUBCONTEXT)
  {
    if (!_sub_context->isReleased())
    {
      _sub_context->tick();
      return false;
    }
    else
    {
      delete _sub_context;
      _sub_context = nullptr;
      _mode = MODE_NORMAL;
      showMainTmpl();
      return true;
    }
  }

  return true;
}

void PrefSelectContext::update()
{
  if (_mode == MODE_SD_UNCONN)
  {
    if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      openContextByID(ID_CONTEXT_MENU);
    }

    return;
  }

  if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    ok();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    openContextByID(ID_CONTEXT_MENU);
  }
  else if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);
    _menu->focusUp();
    _scrollbar->scrollUp();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
    _menu->focusDown();
    _scrollbar->scrollDown();
  }
}

void PrefSelectContext::ok()
{
  uint16_t id = _menu->getCurrItemID();

  if (id == ITEM_ID_AUDIO_MONO)
  {
    ToggleItem* toggle = _menu->getCurrItem()->castTo<ToggleItem>();
    if (toggle->isOn())
    {
      if (SettingsManager::set(STR_PREF_MONO_AUDIO, "0"))
        toggle->setOn(false);
    }
    else
    {
      if (SettingsManager::set(STR_PREF_MONO_AUDIO, "1"))
        toggle->setOn(true);
    }
  }
  else if (id == ITEM_ID_WIFI_AUTOCONN)
  {
    ToggleItem* toggle = _menu->getCurrItem()->castTo<ToggleItem>();
    if (toggle->isOn())
    {
      if (SettingsManager::set(STR_PREF_WIFI_AUTOCONNECT, "0", STR_WIFI_SUBDIR))
        toggle->setOn(false);
    }
    else
    {
      if (SettingsManager::set(STR_PREF_WIFI_AUTOCONNECT, "1", STR_WIFI_SUBDIR))
        toggle->setOn(true);
    }
  }
  else if (id == ITEM_ID_LED_GREET)
  {
    ToggleItem* toggle = _menu->getCurrItem()->castTo<ToggleItem>();

    if (toggle->isOn())
    {
      if (SettingsManager::set(STR_PREF_LED_GREET, "0"))
        toggle->setOn(false);
    }
    else
    {
      if (SettingsManager::set(STR_PREF_LED_GREET, "1"))
        toggle->setOn(true);
    }
  }
  else if (id == ITEM_ID_AUDIO_AMP)
  {
    ToggleItem* toggle = _menu->getCurrItem()->castTo<ToggleItem>();

    if (toggle->isOn())
    {
      if (SettingsManager::set(STR_PREF_AUDIO_AMP, "0"))
        toggle->setOn(false);
    }
    else
    {
      if (SettingsManager::set(STR_PREF_AUDIO_AMP, "1"))
        toggle->setOn(true);
    }
  }
  else if (id == ITEM_ID_BRIGHT)
  {
    _mode = MODE_SUBCONTEXT;
    getLayout()->delWidgets();
    _sub_context = new PrefBrightContext();
  }
  else if (id == ITEM_ID_FILE_SERVER)
  {
    _mode = MODE_SUBCONTEXT;
    getLayout()->delWidgets();
    _sub_context = new PrefFileServerContext();
  }
  else if (id == ITEM_ID_WIFI_POWER)
  {
    _mode = MODE_SUBCONTEXT;
    getLayout()->delWidgets();
    _sub_context = new PrefWiFiPowerContext();
  }
}
