#pragma once

#include "context/IContext.h"
#include "widget/menu/FixedMenu.h"

using namespace pixeler;

class PrefWiFiPowerContext : public IContext
{
public:
  PrefWiFiPowerContext();
  virtual ~PrefWiFiPowerContext() {}

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_HEADER = 1,
    ID_MENU,
    ID_ERR_LBL,
  };

  enum Mode : uint8_t
  {
    MODE_NORMAL = 0,
    MODE_SD_UNCONN
  };

  enum ItemID : uint8_t
  {
    ITEM_ID_MIN = 1,
    ITEM_ID_MID,
    ITEM_ID_MAX
  };

  Mode _mode = MODE_NORMAL;

  FixedMenu* _menu;

  void showSDErrTmpl();
  void showMainTmpl();
};
