#pragma once

#include "context/IContext.h"
#include "widget/menu/FixedMenu.h"
#include "widget/scrollbar/ScrollBar.h"

class AppsContext : public pixeler::IContext
{
public:
  AppsContext();
  virtual ~AppsContext();

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
    ID_ITEM_TEST_BATT = 1,
  };

  pixeler::FixedMenu* _menu;
  pixeler::ScrollBar* _scrollbar;

  void up();
  void down();
  void ok();
};
