#pragma once
#include <stdint.h>

#include "context/IContext.h"
#include "widget/menu/FixedMenu.h"
#include "widget/scrollbar/ScrollBar.h"

using namespace pixeler;

class GameListContext : public IContext
{
public:
  GameListContext();
  virtual ~GameListContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_MENU = 1,
    ID_SCROLLBAR,
  };

  void up();
  void down();

private:
  FixedMenu* _menu;
  ScrollBar* _scrollbar;
};
