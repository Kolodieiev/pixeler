#pragma once

#include "context/IContext.h"
#include "widget/menu/FixedMenu.h"
#include "widget/scrollbar/ScrollBar.h"

using namespace pixeler;

class MenuContext : public IContext
{
public:
  MenuContext();
  virtual ~MenuContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  static void keyPressedHandler(const EspUsbHostKeyboardEvent& event, void* arg);

private:
  enum Widget_ID : uint8_t
  {
    ID_MENU = 1,
    ID_SCROLLBAR,
  };

  FixedMenu* _menu;
  ScrollBar* _scrollbar;

  static uint8_t _last_sel_item_pos;

  void up();
  void down();
  void ok();
};
