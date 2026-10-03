#pragma once

#include "bus/I2C_Bus.h"
#include "context/IContext.h"
#include "lib/ina/219/INA219.h"
#include "widget/menu/FixedMenu.h"
#include "widget/text/Label.h"

class BattTestContext : public pixeler::IContext
{
public:
  BattTestContext();
  virtual ~BattTestContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  using StateHandler = void (BattTestContext::*)();

  enum WidgetID : uint8_t
  {
    ID_STATE_LBL = 1,
    ID_VOLTAGE_LBL,
    ID_CURRENT_LBL,
    ID_POWER_LBL,
    ID_FUN_PWM_LBL,
    ID_OPA_PWM_LBL,
    ID_INSTRUCTION_LBL,
    ID_MAIN_CONTEXT,
    ID_PROFILE_CONTEXT,
    ID_MENU,
    ID_PROF_HINT_LABEL,
  };
  enum ItemID : uint8_t
  {
    ID_ITEM_SELECT_PROFILE = 1,
    ID_ITEM_MANUAL_TEST,
    ID_ITEM_TEMP_PROFILE,

    ID_ITEM_RUN_PROFILE,
    ID_ITEM_EDIT_PROFILE,
    ID_ITEM_DELETE_PROFILE,
    ID_ITEM_NEW_PROFILE,
  };

private:
  void showErrLabelTmpl(const String& msg_str);
  void handleErrState();

  void showMainTmpl();
  void handleMainState();

  void showContextMenu();
  void hideContextMenu();
  void handleContextMenuState();

  void showProfileSelectTmpl();
  void handleProfileSelectState();
  void updateProfileHint();

  void showProfileContextMenu();
  void hideProfileContextMenu();
  void handleProfileContextMenuState();

  void showManualTestTmpl();
  void handleManualTestState();

  void showTestTmpl();
  void handleTestState();

  float readPG();

  void turnOnFun();
  void turnOffFun();

  void turnOnLoad(uint16_t pwm_duty);
  void turnOffLoad();

  void incFunSpeed();
  void decFunSpeed();

  void incOpaPwm();
  void decOpaPwm();

  bool checkVoltageDiff(float bus_voltage, float pg_voltage);

  void updateReadings();
  void updateFunPwmLbl();
  void updateOpaPwmLbl();

  void showProfileEditDialog(const String& profile_name, float min_voltage, float test_current, uint8_t fun_pwm);
  void handleEditProfileState();

private:
  INA219 _ina;
  pixeler::I2C_Bus _i2c;

  StateHandler _state_handler{nullptr};

  pixeler::Label* _voltage_lbl{nullptr};
  pixeler::Label* _current_lbl{nullptr};
  pixeler::Label* _power_lbl{nullptr};

  pixeler::Label* _fun_pwm_lbl{nullptr};
  pixeler::Label* _opa_pwm_lbl{nullptr};

  pixeler::FixedMenu* _menu{nullptr};
  pixeler::FixedMenu* _context_menu{nullptr};

  pixeler::Label* _prof_hint_lbl{nullptr};

  float _min_test_voltage;
  float _test_current = 0.0f;

  unsigned long _readings_upd_ts = 0;

  uint16_t _opa_duty = 0;
  uint8_t _fun_duty = 0;
};
