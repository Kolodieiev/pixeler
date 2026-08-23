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
  enum WidgetID : uint8_t
  {
    ID_MENU = 1,
    ID_SCROLLBAR,
  };

  enum ItemID : uint8_t
  {
    ID_ITEM_FILES = 1,
    ID_ITEM_MP3,
    ID_ITEM_GAMES,
    ID_ITEM_READER,
    ID_ITEM_WIFI,
    ID_ITEM_PREF_SEL,
    ID_ITEM_FIRMWARE,
  };

  FixedMenu* _menu;
  ScrollBar* _scrollbar;

  static uint8_t _last_page_pos;

  void up();
  void down();
  void ok();
};
