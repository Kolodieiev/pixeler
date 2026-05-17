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
  enum Widget_ID : uint8_t
  {
    ID_MENU = 1,
    ID_SCROLLBAR,
  };

  FixedMenu* _menu;
  ScrollBar* _scrollbar;

  void up();
  void down();
  void ok();

private:
  static uint8_t _last_page_pos;
};
