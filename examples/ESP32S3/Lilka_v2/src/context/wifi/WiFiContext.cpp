#include "WiFiContext.h"
//
#include "../WidgetCreator.h"
#include "./res/ico_connect.h"
#include "manager/SettingsManager.h"
#include "widget/layout/EmptyLayout.h"
#include "widget/menu/item/ToggleItem.h"
#include "widget/toggle/ToggleSwitch.h"

const char STR_TRANSMITTER_STATE[] = "Стан модуля WiFi";
const char STR_START_SCAN[] = "Розпочато скануваня";
const char STR_START_SCAN_ERR[] = "Помилка сканування";
const char STR_WIFI_BUSY[] = "Модуль зайнятий";
const char STR_DISCONNECT[] = "Від'єднaтися";
const char STR_FORGET[] = "Забути";
const char STR_CONNECTING[] = "З'єднання...";
const char STR_CONNECT_ERR[] = "Помилка з'єднання";

WiFiContext::WiFiContext()
{
  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  String wifi_power = SettingsManager::get(STR_PREF_WIFI_POWER);

  if (wifi_power.isEmpty())
  {
    _wifi.setPower(WiFiManager::WIFI_POWER_MIN);
  }
  else
  {
    int power_val = std::atoi(wifi_power.c_str());
    _wifi.setPower(static_cast<WiFiManager::WiFiPowerLevel>(power_val));
  }

  showMainTmpl();
}

WiFiContext::~WiFiContext()
{
}

bool WiFiContext::loop()
{
  return true;
}

void WiFiContext::update()
{
  if (_input.isReleased(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, CLICK_LOCK);
    up();
  }
  else if (_input.isReleased(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, CLICK_LOCK);
    down();
  }
  else if (_input.isReleased(BtnID::BTN_LEFT))
  {
    _input.lock(BtnID::BTN_LEFT, CLICK_LOCK);
    left();
  }
  else if (_input.isReleased(BtnID::BTN_RIGHT))
  {
    _input.lock(BtnID::BTN_RIGHT, CLICK_LOCK);
    right();
  }
  else if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    ok();
  }
  else if (_input.isPressed(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, PRESS_LOCK);

    if (_mode == MODE_MAIN)
      showContextMenuTmpl();
    else if (_mode == MODE_ENTER_PWD)
      savePressed();
  }
  else if (_input.isPressed(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, PRESS_LOCK);
    changeKbCaps();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    back();
  }
  else if (_input.isPressed(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, PRESS_LOCK);
    exitPressed();
  }
}

void WiFiContext::showSDErrTmpl()
{
  _mode = MODE_SD_UNCONN;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  layout->addWidget(WidgetCreator::getStatusMsgLable(ID_ERR_LBL, STR_SD_ERR));
}

void WiFiContext::showMainTmpl()
{
  _mode = MODE_MAIN;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _main_menu = new FixedMenu(ID_MAIN_MENU);
  layout->addWidget(_main_menu);
  _main_menu->setWidth(UI_WIDTH);
  _main_menu->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2);
  _main_menu->setPos(0, DISPLAY_CUTOUT);
  _main_menu->setItemHeight(40);
  _main_menu->setLoopState(true);

  // Add state item
  ToggleItem* wifi_state_item = new ToggleItem(ID_ITEM_WIFI_STATE);
  _main_menu->addItem(wifi_state_item);
  wifi_state_item->setFocusBorderColor(COLOR_LIME);
  wifi_state_item->setFocusBackColor(COLOR_FOCUS_BACK);
  wifi_state_item->setChangingBorder(true);
  wifi_state_item->setChangingBack(true);

  Label* wifi_state_lbl = WidgetCreator::getItemLabel(STR_TRANSMITTER_STATE, font_10x20);
  wifi_state_item->setLbl(wifi_state_lbl);

  ToggleSwitch* wifi_state_toggle = new ToggleSwitch(1);
  wifi_state_item->setToggle(wifi_state_toggle);
  wifi_state_toggle->setWidth(40);
  wifi_state_toggle->setHeight(22);
  wifi_state_toggle->setCornerRadius(7);

  if (_wifi.isEnabled())
  {
    addCurrNetItem();
    wifi_state_toggle->setOn(true);
    loadNetsList();
  }
  else
  {
    wifi_state_toggle->setOn(false);
  }
}

void WiFiContext::showEnterPwdTmpl()
{
  _mode = MODE_ENTER_PWD;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _pwd_txt = new TextBox(ID_PWD_TXT);
  layout->addWidget(_pwd_txt);
  _pwd_txt->setHPadding(5);
  _pwd_txt->setWidth(UI_WIDTH - 10);
  _pwd_txt->setHeight(40);
  _pwd_txt->setBackColor(COLOR_WHITE);
  _pwd_txt->setTextColor(COLOR_BLACK);
  _pwd_txt->setTextSize(2);
  _pwd_txt->setPos(5, DISPLAY_CUTOUT);
  _pwd_txt->setCornerRadius(3);

  _keyboard = WidgetCreator::getStandardEnKeyboard(ID_KEYBOARD);
  layout->addWidget(_keyboard);
}

void WiFiContext::addCurrNetItem()
{
  if (!_wifi.isConnected())
    return;

  String ssid_name = _wifi.getSSID();

  MenuItem* cur_net_item = WidgetCreator::getMenuItem(ID_ITEM_CUR_NET);
  _main_menu->addItem(cur_net_item);

  Image* conn_ico = new Image(1);
  cur_net_item->setImg(conn_ico);
  conn_ico->setWidth(16);
  conn_ico->setHeight(16);
  conn_ico->setCornerRadius(1);
  conn_ico->setTransparency(true);
  conn_ico->setSrc(ICO_CONNECT);

  Label* conn_lbl = WidgetCreator::getItemLabel(ssid_name.c_str(), font_10x20);
  cur_net_item->setLbl(conn_lbl);
  conn_lbl->setAutoscrollInFocus(true);
}

void WiFiContext::up()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _keyboard->focusUp();
  }
  else if (_mode == MODE_MAIN)
  {
    _main_menu->focusUp();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    _context_menu->focusUp();
  }
}

void WiFiContext::down()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _keyboard->focusDown();
  }
  else if (_mode == MODE_MAIN)
  {
    _main_menu->focusDown();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    _context_menu->focusDown();
  }
}

void WiFiContext::left()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _keyboard->focusLeft();
  }
}

void WiFiContext::right()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _keyboard->focusRight();
  }
}

void WiFiContext::ok()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _pwd_txt->addChars(_keyboard->getCurrBtnTxt().c_str());
  }
  else if (_mode == MODE_MAIN)
  {
    uint16_t item_id = _main_menu->getCurrItemID();

    if (item_id == ID_ITEM_WIFI_STATE)
    {
      ToggleItem* wifi_state_item = _main_menu->getWidgetByID(ID_ITEM_WIFI_STATE)->castTo<ToggleItem>();
      bool was_enabled = _wifi.isEnabled();

      if (!_wifi.toggle())
      {
        showToast(STR_WIFI_BUSY, TOAST_LENGTH_SHORT);
      }
      else if (!_wifi.isEnabled())
      {
        if (!was_enabled)
        {
          showToast(STR_FAIL, TOAST_LENGTH_SHORT);
        }
        else
        {
          wifi_state_item->setOn(false);
          ToggleItem* temp_toggle = wifi_state_item->clone(ID_ITEM_WIFI_STATE);
          _main_menu->delWidgets();
          _main_menu->addItem(temp_toggle);
        }
      }
      else
      {
        wifi_state_item->setOn(true);
        loadNetsList();
      }
    }
    else if (item_id != ID_ITEM_CUR_NET)
    {
      String ssid = _main_menu->getCurrItemText();
      connectToNet(ssid);
    }
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    uint16_t ctx_item_id = _context_menu->getCurrItemID();
    if (ctx_item_id == ID_ITEM_DISCONN)
    {
      _wifi.disconnect();
      ToggleItem* wifi_state_item = _main_menu->getWidgetByID(ID_ITEM_WIFI_STATE)->castTo<ToggleItem>();
      ToggleItem* temp_toggle = wifi_state_item->clone(ID_ITEM_WIFI_STATE);
      _main_menu->delWidgets();
      _main_menu->addItem(temp_toggle);
      hideContextMenu();
      loadNetsList();
    }
    else if (ctx_item_id == ID_ITEM_FORGET)
    {
      String path_to_pwd = SettingsManager::getSettingsFilePath(_main_menu->getCurrItemText().c_str(), STR_WIFI_SUBDIR);
      if (!_fs.rmFile(path_to_pwd.c_str()))
        showToast(STR_FAIL);
      else
        showToast(STR_SUCCESS);

      hideContextMenu();
    }
  }
}

void WiFiContext::back()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _pwd_txt->removeLastChar();
  }
  else if (_mode == MODE_SD_UNCONN || _mode == MODE_MAIN)
  {
    _wifi.onScanDone(nullptr, nullptr);
    _wifi.onConnectDone(nullptr, nullptr);
    openContextByID(ID_CONTEXT_MENU);
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    hideContextMenu();
  }
}

void WiFiContext::showContextMenuTmpl()
{
  uint16_t item_id = _main_menu->getCurrItemID();
  if (item_id == ID_ITEM_WIFI_STATE)
    return;

  _context_menu = new FixedMenu(ID_CTX_MENU);
  _context_menu->setBackColor(COLOR_MENU_ITEM);
  _context_menu->setBorderColor(COLOR_ORANGE);
  _context_menu->setBorder(true);
  _context_menu->setItemHeight(20);
  _context_menu->setWidth(120);

  if (item_id == ID_ITEM_CUR_NET)
  {
    String ip = _wifi.getIP();

    MenuItem* ip_item = WidgetCreator::getMenuItem(ID_ITEM_IP);
    _context_menu->addItem(ip_item);
    Label* ip_lbl = WidgetCreator::getItemLabel(ip.c_str());
    ip_item->setLbl(ip_lbl);
    ip_lbl->setFullAutoscroll(false);

    MenuItem* disconn_item = WidgetCreator::getMenuItem(ID_ITEM_DISCONN);
    _context_menu->addItem(disconn_item);
    Label* disconn_lbl = WidgetCreator::getItemLabel(STR_DISCONNECT);
    disconn_item->setLbl(disconn_lbl);
  }

  String wifi_pass = SettingsManager::get(_main_menu->getCurrItemText().c_str(), STR_WIFI_SUBDIR);

  if (!wifi_pass.isEmpty())
  {
    MenuItem* forget_item = WidgetCreator::getMenuItem(ID_ITEM_FORGET);
    _context_menu->addItem(forget_item);

    Label* forget_lbl = WidgetCreator::getItemLabel(STR_FORGET);
    forget_item->setLbl(forget_lbl);
  }

  if (_context_menu->getSize() == 0)
  {
    delete _context_menu;
  }
  else
  {
    _main_menu->disable();
    _mode = MODE_CONTEXT_MENU;
    getLayout()->addWidget(_context_menu);
    _context_menu->setHeight(_context_menu->getItemHeight() * _context_menu->getSize() + 4);
    _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight() - DISPLAY_CUTOUT);
  }
}

void WiFiContext::hideContextMenu()
{
  if (_mode != MODE_CONTEXT_MENU)
    return;

  _mode = MODE_MAIN;
  getLayout()->delWidgetByID(ID_CTX_MENU);
  _main_menu->enable();
}

void WiFiContext::changeKbCaps()
{
  if (_mode == MODE_ENTER_PWD)
  {
    _pwd_kb_x_pos = _keyboard->getFocusXPos();
    _pwd_kb_y_pos = _keyboard->getFocusYPos();

    getLayout()->delWidgetByID(ID_KEYBOARD);

    _is_standrad_kb = !_is_standrad_kb;
    if (_is_standrad_kb)
      _keyboard = WidgetCreator::getStandardEnKeyboard(ID_KEYBOARD);
    else
      _keyboard = WidgetCreator::getCapsdEnKeyboard(ID_KEYBOARD);

    _keyboard->setFocusPos(_pwd_kb_x_pos, _pwd_kb_y_pos);
    getLayout()->addWidget(_keyboard);
  }
}

void WiFiContext::savePressed()
{
  if (SettingsManager::set(_sel_ssid.c_str(), _pwd_txt->getText().c_str(), STR_WIFI_SUBDIR))
    connectToNet(_sel_ssid);
  else
    showToast(STR_FAIL, TOAST_LENGTH_LONG);
}

void WiFiContext::exitPressed()
{
  if (_mode == MODE_ENTER_PWD)
  {
    showMainTmpl();
  }
  else if (_mode == MODE_MAIN || _mode == MODE_CONTEXT_MENU)
  {
    back();
  }
}

void WiFiContext::loadNetsList()
{
  _wifi.onScanDone(scanDoneHandler, this);
  if (!_wifi.startScan())
    showToast(STR_START_SCAN_ERR, TOAST_LENGTH_SHORT);
  else
    showToast(STR_START_SCAN, TOAST_LENGTH_SHORT);
}

void WiFiContext::updateNetList(bool no_scan)
{
  if (_mode != MODE_MAIN)
  {
    showMainTmpl();
    return;
  }

  takeLayoutMutex();

  ToggleItem* wifi_state_item = _main_menu->getWidgetByID(ID_ITEM_WIFI_STATE)->castTo<ToggleItem>();
  ToggleItem* temp_toggle = wifi_state_item->clone(ID_ITEM_WIFI_STATE);
  _main_menu->delWidgets();
  _main_menu->addItem(temp_toggle);

  addCurrNetItem();

  bool is_connected = _wifi.isConnected();
  bool curr_scipped = false;
  String cur_ssid = _wifi.getSSID();

  if (!no_scan)
    _wifi.getScanResult(_ssids);

  uint16_t i = ID_ITEM_CUR_NET + 1;
  for (auto i_b = _ssids.begin(), i_e = _ssids.end(); i_b < i_e; ++i_b)
  {
    if (!curr_scipped && is_connected && (i_b)->equals(cur_ssid))
    {
      curr_scipped = true;
      continue;
    }

    MenuItem* item = WidgetCreator::getMenuItem(i);
    _main_menu->addItem(item);

    Label* item_lbl = WidgetCreator::getItemLabel((i_b)->c_str(), font_10x20);
    item->setLbl(item_lbl);
    ++i;
  }

  giveLayoutMutex();
}

void WiFiContext::scanDoneHandler(void* arg)
{
  WiFiContext* self = static_cast<WiFiContext*>(arg);
  self->updateNetList();
}

void WiFiContext::connectToNet(const String& ssid)
{
  String wifi_pass = SettingsManager::get(ssid.c_str(), STR_WIFI_SUBDIR);

  if (wifi_pass.isEmpty())
  {
    _sel_ssid = _main_menu->getCurrItemText();
    showEnterPwdTmpl();
  }
  else
  {
    _wifi.onConnectDone(connDoneHandler, this);

    String wifi_autoconn = SettingsManager::get(STR_PREF_WIFI_AUTOCONNECT, STR_WIFI_SUBDIR);
    if (wifi_autoconn.equals("1"))
      SettingsManager::set(STR_PREF_WIFI_LAST_SSID, ssid.c_str(), STR_WIFI_SUBDIR);

    if (!_wifi.tryConnectTo(ssid, wifi_pass))
      showToast(STR_FAIL, TOAST_LENGTH_SHORT);
    else
      showToast(STR_CONNECTING, TOAST_LENGTH_SHORT);
  }
}

void WiFiContext::connDoneHandler(void* arg, wl_status_t conn_status)
{
  WiFiContext* self = static_cast<WiFiContext*>(arg);
  if (conn_status != WL_CONNECTED)
    self->showToast(STR_CONNECT_ERR, TOAST_LENGTH_LONG);
  else
  {
    self->showToast(STR_SUCCESS, TOAST_LENGTH_SHORT);
    self->updateNetList(true);
  }
}
