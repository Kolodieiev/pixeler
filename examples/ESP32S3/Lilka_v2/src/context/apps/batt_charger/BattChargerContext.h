#pragma once

#include "bus/I2C_Bus.h"
#include "context/IContext.h"
#include "lib/ads/1X15/Adafruit_ADS1X15.h"
#include "lib/ina/219/INA219.h"
#include "widget/keyboard/Keyboard.h"
#include "widget/menu/FixedMenu.h"
#include "widget/spinbox/SpinBox.h"
#include "widget/text/Label.h"
#include "widget/text/TextBox.h"

class BattChargerContext : public pixeler::IContext
{
public:
  BattChargerContext();
  virtual ~BattChargerContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  using StateHandler = void (BattChargerContext::*)();

  enum BatType : uint8_t
  {
    BATT_TYPE_LIP = 0,
    BATT_TYPE_HVLIP,
    BATT_TYPE_LIFEPO,
  };

  struct ProfileSetup
  {
    float max_voltage = 0.0f;
    float max_current = 0.0f;
    BatType batt_type = BATT_TYPE_LIP;
  };

  struct BatChargeProfile
  {
    float max_voltage = 0.0f;
    float min_voltage = 0.0f;
  };

  enum WidgetID : uint8_t
  {
    ID_STATE_LBL = 1,
    ID_MAX_VOLTAGE_LBL,
    ID_MAX_CURRENT_LBL,
    ID_POWER_LBL,
    ID_CAPACITY_LBL,
    ID_CHARGE_TIME_LBL,
    ID_INSTRUCTION_LBL,
    ID_C_MENU,
    ID_MENU,
    ID_PROF_HINT_LBL,

    ID_PROF_NAME_NAME_LBL,
    ID_PROF_MAX_VOLT_LBL,
    ID_PROF_MAX_CURR_LBL,

    ID_PROF_NAME_TB,
    ID_PROF_NAME_KB,

    ID_PROF_EDIT_TITILE_LBL,
    ID_PROF_SPINBOX,
  };

  enum ItemID : uint8_t
  {
    ID_ITEM_TEMP_PROFILE = 1,
    ID_ITEM_RUN_PROFILE,
    ID_ITEM_EDIT_PROFILE,
    ID_ITEM_DELETE_PROFILE,
    ID_ITEM_NEW_PROFILE,

    ID_ITEM_PROF_EDIT_NAME,
    ID_ITEM_PROF_EDIT_VOLT,
    ID_ITEM_PROF_EDIT_CURR,
    ID_ITEM_PROF_EDIT_BATT_TYPE,

    ID_ITEM_LIP,
    ID_ITEM_HVLIP,
    ID_ITEM_LIFEPO,
  };

  void showErrLabelTmpl(const String& msg_str);
  void handleErrState();

  void showMainTmpl();
  void handleMainState();
  void updateProfileHint();

  void showProfileContextMenu();
  void hideProfileContextMenu();
  void handleProfileContextMenuState();

  void showChargeTmpl();
  void handleChargeState();

  void showProfileEditTmpl();
  void handleProfileEditState();

  void showProfileNameDialog();
  void handleProfileNameDialogState();

  void showProfileMaxVoltDialog();
  void handleProfileMaxVoltDialogState();

  void showProfileMaxCurrDialog();
  void handleProfileMaxCurrDialogState();

  void showProfileBatTypeDialog();
  void handleProfileBatTypeDialogState();

  void updateVoltages();

  void turnOnDcIn();
  void turnOffDcIn();

  void turnOnDcOut();
  void turnOffDcOut();

  void incVoltagePwm();
  void decVoltagePwm();

  bool checkOutVoltageDiff();

  void updateReadings();

  void updateCapacity(float current);
  void updateChargeTime();

  void adjustOutVoltagePWM();

  void addInstruction(const String& text, uint8_t line_num);

  const char* getNameByBattType(BatType batt_type);

  static void wdt_task(void* arg);

private:
  String _profile_edit_name;  // Глобальна змінна для простого перенесення значення між вікнами GUI
  INA219 _ina;
  Adafruit_ADS1X15 _ads;
  StateHandler _state_handler{nullptr};

  ProfileSetup _profile;
  ProfileSetup _profile_edit;  // Глобальна змінна для простого перенесення значення між вікнами GUI

  double _capacity_mah = 0.0;

  float _prev_current = 0.0f;
  float _ads_out_voltage = 0.0f;
  float _ina_out_voltage = 0.0f;
  float _bat_voltage = 0.0f;

  TaskHandle_t _wdt_task_handle{nullptr};
  pixeler::Label* _voltage_lbl{nullptr};
  pixeler::Label* _current_lbl{nullptr};
  pixeler::Label* _power_lbl{nullptr};
  pixeler::Label* _capacity_lbl{nullptr};
  pixeler::Label* _test_time_lbl{nullptr};

  pixeler::FixedMenu* _menu{nullptr};
  pixeler::FixedMenu* _context_menu{nullptr};

  pixeler::Label* _prof_hint_lbl{nullptr};

  pixeler::TextBox* _profile_name_tb{nullptr};
  pixeler::Keyboard* _keyboard{nullptr};

  pixeler::SpinBox* _spinbox{nullptr};

  unsigned long _readings_upd_ts = 0;
  unsigned long _test_start_ts = 0;
  unsigned long _upd_test_time_ts = 0;

  uint16_t _vout_pwm_duty = 0;

  bool _was_wdt_fed = false;
};
