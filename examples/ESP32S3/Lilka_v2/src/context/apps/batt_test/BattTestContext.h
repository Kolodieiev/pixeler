#pragma once

#include "bus/I2C_Bus.h"
#include "context/IContext.h"
#include "lib/ina/219/INA219.h"
#include "widget/keyboard/Keyboard.h"
#include "widget/menu/FixedMenu.h"
#include "widget/spinbox/SpinBox.h"
#include "widget/text/Label.h"
#include "widget/text/TextBox.h"

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

  struct ProfileSetup
  {
    float min_voltage = 0.0f;
    float max_current = 0.0f;
    uint8_t fun_duty = 0;
  };

  enum WidgetID : uint8_t
  {
    ID_STATE_LBL = 1,
    ID_VOLTAGE_LBL,
    ID_CURRENT_LBL,
    ID_POWER_LBL,
    ID_FUN_PWM_LBL,
    ID_OPA_PWM_LBL,
    ID_INSTRUCTION_LBL,
    ID_C_MENU,
    ID_MENU,
    ID_PROF_HINT_LBL,

    ID_PROF_NAME_NAME_LBL,
    ID_PROF_MIN_VOLT_LBL,
    ID_PROF_MAX_CURR_LBL,
    ID_PROF_FUN_PWM_LBL,

    ID_PROF_NAME_TB,
    ID_PROF_NAME_KB,

    ID_PROF_EDIT_TITILE_LBL,
    ID_PROF_SPINBOX,
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

    ID_PROF_EDIT_NAME_ITEM,
    ID_PROF_EDIT_VOLT_ITEM,
    ID_PROF_EDIT_CURR_ITEM,
    ID_PROF_EDIT_FUN_ITEM,
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

  void showProfileEditTmpl();
  void handleProfileEditState();

  void showProfileNameDialog();
  void handleProfileNameDialogState();

  void showProfileMinVoltDialog();
  void handleProfileMinVoltDialogState();

  void showProfileMaxCurrDialog();
  void handleProfileMaxCurrDialogState();

  void showProfileFunPwmDialog();
  void handleProfileFunPwmDialogState();

  void addInstruction(const String& text, uint8_t line_num);

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

private:
  INA219 _ina;

  String _profile_edit_name;  // Глобальна змінна для простого перенесення значення між вікнами GUI

  StateHandler _state_handler{nullptr};

  pixeler::Label* _voltage_lbl{nullptr};
  pixeler::Label* _current_lbl{nullptr};
  pixeler::Label* _power_lbl{nullptr};

  pixeler::Label* _fun_pwm_lbl{nullptr};
  pixeler::Label* _opa_pwm_lbl{nullptr};

  pixeler::FixedMenu* _menu{nullptr};
  pixeler::FixedMenu* _context_menu{nullptr};

  pixeler::Label* _prof_hint_lbl{nullptr};

  pixeler::TextBox* _profile_name_tb{nullptr};
  pixeler::Keyboard* _keyboard{nullptr};

  pixeler::SpinBox* _spinbox{nullptr};

  float _min_test_voltage;
  float _test_current = 0.0f;

  float _profile_edit_min_voltage;  // Глобальна змінна для простого перенесення значення між вікнами GUI
  float _profile_edit_max_current;  // Глобальна змінна для простого перенесення значення між вікнами GUI

  unsigned long _readings_upd_ts = 0;

  uint16_t _opa_duty = 0;
  uint8_t _fun_duty = 0;

  uint8_t _profile_edit_fun_pwm;  // Глобальна змінна для простого перенесення значення між вікнами GUI

  pixeler::I2C_Bus _i2c;
};
