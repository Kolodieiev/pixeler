#pragma once

#include "manager/WiFiManager.h"
#include "context/IContext.h"
#include "widget/keyboard/Keyboard.h"
#include "widget/menu/FixedMenu.h"
#include "widget/scrollbar/ScrollBar.h"
#include "widget/text/Label.h"
#include "widget/text/TextBox.h"

using namespace pixeler;

class WiFiContext : public IContext
{
public:
  WiFiContext();
  virtual ~WiFiContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_MAIN_MENU = 1,
    ID_CTX_MENU,
    ID_ERR_LBL,
    ID_PWD_TXT,
    ID_KEYBOARD
  };

  enum Mode : uint8_t
  {
    MODE_MAIN = 0,
    MODE_SD_UNCONN,
    MODE_CONTEXT_MENU,
    MODE_ENTER_PWD
  };

  enum ItemId : uint8_t
  {
    ID_ITEM_WIFI_STATE = 1,
    ID_ITEM_IP,
    ID_ITEM_DISCONN,
    ID_ITEM_FORGET,
    ID_ITEM_CUR_NET  // Повинен бути завжди останнім в перечисленні
  };
  //
  void showSDErrTmpl();
  void showMainTmpl();
  void showEnterPwdTmpl();
  void showContextMenuTmpl();
  void hideContextMenu();
  //
  void addCurrNetItem();
  //
  void up();
  void down();
  void left();
  void right();
  void ok();
  void back();
  void changeKbCaps();
  void savePressed();
  void exitPressed();
  //
  void loadNetsList();
  void updateNetList(bool no_scan = false);
  void connectToNet(const String& ssid);
  //
  static void scanDoneHandler(void* arg);
  static void connDoneHandler(void* arg, wl_status_t conn_status);

private:
  String _sel_ssid;
  std::vector<String> _ssids;

  FixedMenu* _main_menu;
  FixedMenu* _context_menu;
  TextBox* _pwd_txt;
  Keyboard* _keyboard;

  uint16_t _pwd_kb_x_pos = 0;
  uint16_t _pwd_kb_y_pos = 0;

  Mode _mode = MODE_MAIN;

  bool _is_standrad_kb = true;
};
