#include "BattTestContext.h"

#include <esp32-hal-ledc.h>
#include <math.h>

#include <vector>

#include "../../WidgetCreator.h"
#include "context/apps/AppsContext.h"
#include "manager/SettingsManager.h"
#include "util/volt_util.h"

using namespace pixeler;

static const char STR_ERR_INA219[] = "INA219 не відповідає";
static const char STR_ERR_MODULE_BROKEN[] = "Модуль пошкоджено, негайно від'єднайте АКБ!";
static const char STR_ERR_VOLT_DIFF[] = "Помилка читання напруги АКБ";

static const char STR_MAIN_HINT[] =
    "Click A -> Контекстне меню\n"
    "Press B -> Вихід";
static const char STR_MANUAL_TEST_HINT[] =
    "Press B -> Вихід\n"
    "Hold Left/Right -> Змінити навантаження\n"
    "Hold Up/Down -> Змінити швидкість вентилятора";
static const char STR_PROFILE_EDIT_HINT[] =
    "Press A -> Зберегти\n"
    "Press B -> Вихід";
static const char STR_PROFILE_EDIT_SPIN_HINT[] =
    "Hold UP/DOWN -> Змінити значення\n"
    "Click A -> Зафіксувати\n"
    "Click B -> Вихід";

static const char STR_ITEM_SEL_PROFILE[] = "Обрати профіль тестування";
static const char STR_ITEM_MANUAL_TESTING[] = "Ручне тестування";

static const char STR_ITEM_TEMP_PROFILE[] = "Тимчасовий профіль";
static const char STR_ITEM_RUN_PROFILE[] = "Запустити тест";
static const char STR_ITEM_NEW_PROFILE[] = "Новий профіль";

static const char STR_ITEM_PROFILE_NAME[] = "Ім'я: ";
static const char STR_ITEM_PROFILE_VOLT[] = "Мін. V: ";
static const char STR_ITEM_PROFILE_CURR[] = "Макс. C: ";
static const char STR_ITEM_PROFILE_FUN[] = "FUN ШІМ: ";

static const char STR_VOLTAGE_TITLE[] = "Мінімальна напруга";
static const char STR_CURRENT_TITLE[] = "Максимальний струм";
static const char STR_FUN_TITLE[] = "Шпаруватість вентилятора";

static const char STR_ERR_INCORRECT_PROF_NAME[] = "Некоректне ім'я";

static const char STR_APP_DIR[] = "batt_tester";

static const float INA_MAX_CURR = 3.2f;  // Максимальний струм можливий для шунта INA219
static const float INA_SHUNT_R = 0.1f;   // Значення опору шунта INA219

static const float MIN_WARNING_CURRENT = 0.035f;  // Мінімальний струм спокою, при якому модуль вважається пошкодженим
static const float MIN_PG_VOLT = 0.4f;            // Мінімальна напруга на піні PG при якому акум вважається підключеним
static const float MIN_TEST_VOLT = 0.4f;          // Мінімальна можлива напруга тестування акума
static const float MAX_TEST_VOLT = 12.6f;
static const float MAX_VOLT_DIFF = 0.3f;  // Максимальне допустиме значення розбіжності напруг на модулі та PG

static const float R_DIV = 0.0917f;  // Точний коефіцієнт подільника на піні PG

static const float TEMP_PROF_MIN_VOLT = 3.0f;  // Мінімальна напруга тимчасового профіля

static const unsigned long UPD_READINGS_DELAY = 100LU;  // Затримка між оновленням показників в UI

static const uint32_t FUN_PWM_FREQ = 40000;
static const uint32_t OPA_PWM_FREQ = 17000;

static const uint16_t SAMPLES_NUM = 128;
static const uint16_t MAX_OPA_DUTY = 4096;

static const uint8_t INA_ADDR = 0x40;  // Адреса модуля INA219

static const uint8_t PWM_FUN_RES = 8;
static const uint8_t MAX_FUN_DUTY = 100;

static const uint8_t PWM_OPA_RES = 12;

static const uint8_t PIN_PG = 14;
static const uint8_t PIN_FUN_PWM = 13;
static const uint8_t PIN_OPA_PWM = 12;

bool BattTestContext::loop()
{
  return true;
}

void BattTestContext::update()
{
  (this->*_state_handler)();
}

BattTestContext::~BattTestContext()
{
  ledcWrite(PIN_FUN_PWM, 0);
  ledcWrite(PIN_OPA_PWM, 0);
  ledcDetach(PIN_FUN_PWM);
  ledcDetach(PIN_OPA_PWM);

  _i2c.end();
}

BattTestContext::BattTestContext() : _ina{INA_ADDR},
                                     _min_test_voltage{TEMP_PROF_MIN_VOLT}
{
  pinMode(PIN_PG, INPUT);

  ledcAttach(PIN_FUN_PWM, FUN_PWM_FREQ, PWM_FUN_RES);
  ledcAttach(PIN_OPA_PWM, OPA_PWM_FREQ, PWM_OPA_RES);

  turnOnFun();
  turnOffLoad();

  float pg_voltage = readPG();

  _i2c.begin();
  if (!_ina.begin())
  {
    log_e("Модуль INA219 не відповідає за вказаною адресою: 0x%02X", INA_ADDR);
    if (pg_voltage > MIN_PG_VOLT)
      showErrLabelTmpl(STR_ERR_MODULE_BROKEN);  // не можемо дізнатися чи є КЗ
    else
      showErrLabelTmpl(STR_ERR_INA219);

    return;
  }

  _ina.setMaxCurrentShunt(INA_MAX_CURR, INA_SHUNT_R);
  _ina.setShuntSamples(5);
  _ina.setBusSamples(5);

  float current = _ina.getCurrent();
  if (current > MIN_WARNING_CURRENT)
  {
    log_e("Виявлено витік струму в стані спокою: %f A", current);
    showErrLabelTmpl(STR_ERR_MODULE_BROKEN);
    return;
  }

  if (!checkVoltageDiff(_ina.getBusVoltage(), pg_voltage))
    return;

  turnOffFun();

  showMainTmpl();
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showErrLabelTmpl(const String& msg_str)
{
  _state_handler = &BattTestContext::handleErrState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* state_lbl = new Label(ID_STATE_LBL);
  layout->addWidget(state_lbl);

  state_lbl->setText(msg_str);
  state_lbl->setFont(font_inr24);
  state_lbl->setWidth(UI_WIDTH);
  state_lbl->setMultiline(true);
  state_lbl->setHeight(state_lbl->getCharHgt() * 4);
  state_lbl->setPos(0, getCenterY(state_lbl));
  state_lbl->setBackColor(layout->getBackColor());
  state_lbl->setTextColor(COLOR_RED);
}

void BattTestContext::handleErrState()
{
  if (_input.isReleased(BTN_BACK))
    openContext(new AppsContext());
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::addInstruction(const String& text, uint8_t line_num)
{
  Label* instruction_lbl = new Label(ID_INSTRUCTION_LBL);
  getLayout()->addWidget(instruction_lbl);
  instruction_lbl->setText(text);
  instruction_lbl->setFont(font_6x12);
  instruction_lbl->setWidth(UI_WIDTH - DISPLAY_CUTOUT * 2);
  instruction_lbl->setMultiline(true);
  instruction_lbl->setHeight(instruction_lbl->getCharHgt() * line_num);
  instruction_lbl->setPos(DISPLAY_CUTOUT, UI_HEIGHT - instruction_lbl->getHeight());
  instruction_lbl->setBackColor(getLayout()->getBackColor());
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showMainTmpl()
{
  _state_handler = &BattTestContext::handleMainState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _voltage_lbl = new Label(ID_VOLTAGE_LBL);
  layout->addWidget(_voltage_lbl);

  _voltage_lbl->setText(STR_EMPTY_BAT);
  _voltage_lbl->setFont(font_inr30);
  _voltage_lbl->setWidth(UI_WIDTH);
  _voltage_lbl->setPos(0, getCenterY(_voltage_lbl));
  _voltage_lbl->setBackColor(layout->getBackColor());
  _voltage_lbl->setTextColor(COLOR_RED);
  _voltage_lbl->setAlign(IWidget::ALIGN_CENTER);
  _voltage_lbl->setGravity(IWidget::GRAVITY_CENTER);

  addInstruction(STR_MAIN_HINT, 3);
}

void BattTestContext::handleMainState()
{
  if (millis() - _readings_upd_ts > UPD_READINGS_DELAY)
  {
    float voltage = _ina.getBusVoltage();
    String volt_str = String(voltage);
    volt_str += "V";
    _voltage_lbl->setText(volt_str);

    _readings_upd_ts = millis();
  }

  if (_input.isPressed(BTN_BACK))
  {
    openContext(new AppsContext());
    return;
  }

  if (_input.isReleased(BTN_OK))
    showContextMenu();
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showContextMenu()
{
  _state_handler = &BattTestContext::handleContextMenuState;

  _context_menu = WidgetCreator::getContextMenu(ID_C_MENU);
  getLayout()->addWidget(_context_menu);

  // Обрати профіль тестування
  MenuItem* sel_profile_item = WidgetCreator::getMenuItem(ID_ITEM_SELECT_PROFILE);
  _context_menu->addItem(sel_profile_item);

  Label* sel_profile_lbl = WidgetCreator::getItemLabel(STR_ITEM_SEL_PROFILE, font_unifont);
  sel_profile_item->setLabel(sel_profile_lbl);
  sel_profile_lbl->setHPadding(1);

  // Ручне тестування
  MenuItem* manual_test_item = WidgetCreator::getMenuItem(ID_ITEM_MANUAL_TEST);
  _context_menu->addItem(manual_test_item);

  Label* manual_test_lbl = WidgetCreator::getItemLabel(STR_ITEM_MANUAL_TESTING, font_unifont);
  manual_test_item->setLabel(manual_test_lbl);
  manual_test_lbl->setHPadding(1);

  _context_menu->setHeight(_context_menu->getItemHeight() * _context_menu->getSize() + 4);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight() - DISPLAY_CUTOUT);
}

void BattTestContext::hideContextMenu()
{
  getLayout()->delWidgetByID(ID_C_MENU);
  _state_handler = &BattTestContext::handleMainState;
}

void BattTestContext::handleContextMenuState()
{
  if (_input.isReleased(BTN_UP))
  {
    _context_menu->focusUp();
  }
  else if (_input.isReleased(BTN_DOWN))
  {
    _context_menu->focusDown();
  }
  else if (_input.isReleased(BTN_BACK))
  {
    hideContextMenu();
  }
  else if (_input.isReleased(BTN_OK))
  {
    uint16_t item_id = _context_menu->getCurrItemID();
    switch (item_id)
    {
      case ID_ITEM_SELECT_PROFILE:
        showProfileSelectTmpl();
        break;

      case ID_ITEM_MANUAL_TEST:
        showManualTestTmpl();
        break;

      default:
        log_e("Не реалізована функція");
        break;
    }
  }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showProfileSelectTmpl()
{
  _state_handler = &BattTestContext::handleProfileSelectState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MAIN_BACK);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - DISPLAY_PADDING * 2);
  _menu->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2 - DISPLAY_PADDING * 2);
  _menu->setItemHeight(_menu->getHeight() / MENU_ITEMS_NUM - 2);
  _menu->setPos(DISPLAY_PADDING, DISPLAY_CUTOUT + DISPLAY_PADDING);
  _menu->setLooped(true);

  _prof_hint_lbl = new Label(ID_PROF_HINT_LBL);
  layout->addWidget(_prof_hint_lbl);
  _prof_hint_lbl->setFont(font_8x13);
  _prof_hint_lbl->setWidth(UI_WIDTH - DISPLAY_CUTOUT * 2);
  _prof_hint_lbl->setPos(DISPLAY_CUTOUT, UI_HEIGHT - _prof_hint_lbl->getHeight());
  _prof_hint_lbl->setAlign(IWidget::ALIGN_CENTER);
  _prof_hint_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _prof_hint_lbl->setBackColor(layout->getBackColor());

  // Тимчасовий профіль
  MenuItem* temp_item = WidgetCreator::getMenuItem(ID_ITEM_TEMP_PROFILE);
  _menu->addItem(temp_item);

  Label* temp_lbl = WidgetCreator::getItemLabel(STR_ITEM_TEMP_PROFILE, font_10x20);
  temp_item->setLabel(temp_lbl);

  updateProfileHint();

  if (!_fs.isMounted())
    return;

  String dir_path = SettingsManager::getSettingsDirPath(STR_APP_DIR);
  std::vector<FileInfo> profiles = _fs.indexFiles(dir_path.c_str());

  uint16_t i = ID_ITEM_TEMP_PROFILE + 1;
  for (const FileInfo& f_info : profiles)
  {
    if (!f_info.isDir())
    {
      MenuItem* item = WidgetCreator::getMenuItem(i);
      _menu->addItem(item);

      Label* itemp_lbl = WidgetCreator::getItemLabel(f_info.getName(), font_10x20);
      item->setLabel(itemp_lbl);
      ++i;
    }
  }
}

void BattTestContext::handleProfileSelectState()
{
  if (_input.isReleased(BTN_BACK))
  {
    showMainTmpl();
    return;
  }

  if (_input.isReleased(BtnID::BTN_OK))
  {
    showProfileContextMenu();
  }
  else if (_input.isReleased(BtnID::BTN_UP))
  {
    _menu->focusUp();
    updateProfileHint();
  }
  else if (_input.isReleased(BtnID::BTN_DOWN))
  {
    _menu->focusDown();
    updateProfileHint();
  }
}

void BattTestContext::updateProfileHint()
{
  uint16_t item_id = _menu->getCurrItemID();

  float hint_voltage = 999.0f;
  float hint_current = 0.0f;
  uint8_t hint_fun = 0;

  if (item_id == ID_ITEM_TEMP_PROFILE)
  {
    hint_voltage = _min_test_voltage;
    hint_current = _test_current;
    hint_fun = _fun_duty;
  }
  else
  {
    ProfileSetup prof_setup;
    if (SettingsManager::load(&prof_setup, sizeof(prof_setup), _menu->getCurrItemText(), STR_APP_DIR))
    {
      hint_voltage = prof_setup.min_voltage;
      hint_current = prof_setup.max_current;
      hint_fun = prof_setup.fun_duty;
    }
  }

  String hint_str = String(hint_voltage);
  hint_str += "V  ";
  hint_str += hint_current;
  hint_str += "A  ";
  hint_str += "Fun: ";
  hint_str += hint_fun;

  _prof_hint_lbl->setText(hint_str);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showProfileContextMenu()
{
  _state_handler = &BattTestContext::handleProfileContextMenuState;

  _context_menu = WidgetCreator::getContextMenu(ID_C_MENU);
  getLayout()->addWidget(_context_menu);

  // Запустити тест
  MenuItem* run_profile_item = WidgetCreator::getMenuItem(ID_ITEM_RUN_PROFILE);
  _context_menu->addItem(run_profile_item);

  Label* run_profile_lbl = WidgetCreator::getItemLabel(STR_ITEM_RUN_PROFILE, font_unifont);
  run_profile_item->setLabel(run_profile_lbl);
  run_profile_lbl->setHPadding(1);

  // Редагувати профіль
  MenuItem* edit_profile_item = WidgetCreator::getMenuItem(ID_ITEM_EDIT_PROFILE);
  _context_menu->addItem(edit_profile_item);

  Label* edit_profile_lbl = WidgetCreator::getItemLabel(STR_EDIT, font_unifont);
  edit_profile_item->setLabel(edit_profile_lbl);
  edit_profile_lbl->setHPadding(1);

  if (_menu->getCurrItemID() != ID_ITEM_TEMP_PROFILE)
  {
    // Видалити
    MenuItem* delete_profile_item = WidgetCreator::getMenuItem(ID_ITEM_DELETE_PROFILE);
    _context_menu->addItem(delete_profile_item);

    Label* delete_profile_lbl = WidgetCreator::getItemLabel(STR_DELETE, font_unifont);
    delete_profile_item->setLabel(delete_profile_lbl);
    delete_profile_lbl->setHPadding(1);
  }

  if (_fs.isMounted())
  {
    // Новий профіль
    MenuItem* new_profile_item = WidgetCreator::getMenuItem(ID_ITEM_NEW_PROFILE);
    _context_menu->addItem(new_profile_item);

    Label* new_profile_lbl = WidgetCreator::getItemLabel(STR_ITEM_NEW_PROFILE, font_unifont);
    new_profile_item->setLabel(new_profile_lbl);
    new_profile_lbl->setHPadding(1);
  }

  _context_menu->setHeight(_context_menu->getItemHeight() * _context_menu->getSize() + 4);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight() - DISPLAY_CUTOUT);
}

void BattTestContext::hideProfileContextMenu()
{
  getLayout()->delWidgetByID(ID_C_MENU);
  _state_handler = &BattTestContext::handleProfileSelectState;
}

void BattTestContext::handleProfileContextMenuState()
{
  if (_input.isReleased(BTN_UP))
  {
    _context_menu->focusUp();
  }
  else if (_input.isReleased(BTN_DOWN))
  {
    _context_menu->focusDown();
  }
  else if (_input.isReleased(BTN_BACK))
  {
    hideProfileContextMenu();
  }
  else if (_input.isReleased(BTN_OK))
  {
    uint16_t item_id = _context_menu->getCurrItemID();
    switch (item_id)
    {
      case ID_ITEM_RUN_PROFILE:
      {
        String profile_name = _menu->getCurrItemText();

        bool result = true;
        if (!profile_name.equals(STR_ITEM_TEMP_PROFILE))
        {
          ProfileSetup prof_setup;
          result = SettingsManager::load(&prof_setup, sizeof(prof_setup), _menu->getCurrItemText(), STR_APP_DIR);
          if (!result)
          {
            showToast(STR_FAIL);
          }
          else
          {
            _min_test_voltage = prof_setup.min_voltage;
            _max_current = prof_setup.max_current;
            _fun_duty = prof_setup.fun_duty;
          }
        }

        if (result)
          showTestTmpl();
      }

      break;

      case ID_ITEM_DELETE_PROFILE:
        if (!SettingsManager::remove(_menu->getCurrItemText(), STR_APP_DIR))
        {
          showToast(STR_FAIL);
        }
        else
        {
          showProfileSelectTmpl();
          showToast(STR_SUCCESS);
        }
        break;

      case ID_ITEM_EDIT_PROFILE:
      {
        _profile_edit_name = _menu->getCurrItemText();

        ProfileSetup prof_setup;
        if (!_profile_edit_name.equals(STR_ITEM_TEMP_PROFILE))
        {
          if (!SettingsManager::load(&prof_setup, sizeof(prof_setup), _profile_edit_name, STR_APP_DIR))
          {
            showToast(STR_FAIL);
          }
          else
          {
            _profile_edit_min_voltage = prof_setup.min_voltage;
            _profile_edit_max_current = prof_setup.max_current;
            _profile_edit_fun_pwm = prof_setup.fun_duty;
          }
        }
        else
        {
          _profile_edit_min_voltage = _min_test_voltage;
          _profile_edit_max_current = _test_current;
          _profile_edit_fun_pwm = _fun_duty;
        }

        showProfileEditTmpl();
      }
      break;

      case ID_ITEM_NEW_PROFILE:
        _profile_edit_name = STR_ITEM_NEW_PROFILE;
        _profile_edit_min_voltage = _min_test_voltage;
        _profile_edit_max_current = _test_current;
        _profile_edit_fun_pwm = _fun_duty;
        showProfileEditTmpl();
        break;

      default:
        log_e("Не реалізована функція");
        break;
    }
  }
}

void BattTestContext::showProfileEditTmpl()
{
  _state_handler = &BattTestContext::handleProfileEditState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _menu = new FixedMenu(ID_MENU);
  layout->addWidget(_menu);
  _menu->setBackColor(COLOR_MAIN_BACK);
  _menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH - DISPLAY_PADDING * 2);
  _menu->setHeight(UI_HEIGHT - DISPLAY_CUTOUT * 2 - DISPLAY_PADDING * 2);
  _menu->setItemHeight(_menu->getHeight() / MENU_ITEMS_NUM - 2);
  _menu->setPos(DISPLAY_PADDING, DISPLAY_CUTOUT + DISPLAY_PADDING);
  _menu->setLooped(true);

  addInstruction(STR_PROFILE_EDIT_HINT, 3);

  // Ім'я профіля
  MenuItem* prof_name_item = WidgetCreator::getMenuItem(ID_PROF_EDIT_NAME_ITEM);
  _menu->addItem(prof_name_item);

  String prof_name_text = STR_ITEM_PROFILE_NAME;
  prof_name_text += _profile_edit_name;
  Label* prof_name_lbl = WidgetCreator::getItemLabel(prof_name_text, font_10x20);
  prof_name_item->setLabel(prof_name_lbl);

  // Мінімальна напруга
  MenuItem* prof_volt_item = WidgetCreator::getMenuItem(ID_PROF_EDIT_VOLT_ITEM);
  _menu->addItem(prof_volt_item);

  String prof_volt_text = STR_ITEM_PROFILE_VOLT;
  prof_volt_text += _profile_edit_min_voltage;
  Label* prof_volt_lbl = WidgetCreator::getItemLabel(prof_volt_text, font_10x20);
  prof_volt_item->setLabel(prof_volt_lbl);

  // Максимальний струм
  MenuItem* prof_curr_item = WidgetCreator::getMenuItem(ID_PROF_EDIT_CURR_ITEM);
  _menu->addItem(prof_curr_item);

  String prof_curr_text = STR_ITEM_PROFILE_CURR;
  prof_curr_text += _profile_edit_max_current;
  Label* prof_curr_lbl = WidgetCreator::getItemLabel(prof_curr_text, font_10x20);
  prof_curr_item->setLabel(prof_curr_lbl);

  // ШІМ вентилятора
  MenuItem* prof_fun_item = WidgetCreator::getMenuItem(ID_PROF_EDIT_FUN_ITEM);
  _menu->addItem(prof_fun_item);

  String prof_fun_text = STR_ITEM_PROFILE_FUN;
  prof_fun_text += _profile_edit_fun_pwm;
  Label* prof_fun_lbl = WidgetCreator::getItemLabel(prof_fun_text, font_10x20);
  prof_fun_item->setLabel(prof_fun_lbl);
}

void BattTestContext::handleProfileEditState()
{
  if (_input.isPressed(BTN_BACK))
  {
    showProfileSelectTmpl();
    return;
  }

  if (_input.isPressed(BTN_OK))
  {
    if (_profile_edit_name.equals(STR_ITEM_TEMP_PROFILE))
    {
      _min_test_voltage = _profile_edit_min_voltage;
      _test_current = _profile_edit_max_current;
      _fun_duty = _profile_edit_fun_pwm;
      showToast(STR_SUCCESS);
    }
    else
    {
      ProfileSetup prof_setup{
          .min_voltage = _profile_edit_min_voltage,
          .max_current = _profile_edit_max_current,
          .fun_duty = _profile_edit_fun_pwm};

      if (!SettingsManager::save(&prof_setup, sizeof(prof_setup), _profile_edit_name, STR_APP_DIR))
      {
        showToast(STR_FAIL);
      }
      else
      {
        showProfileSelectTmpl();
        showToast(STR_SUCCESS);
      }
    }

    showProfileSelectTmpl();
    return;
  }

  if (_input.isReleased(BTN_UP))
  {
    _menu->focusUp();
  }
  else if (_input.isReleased(BTN_DOWN))
  {
    _menu->focusDown();
  }
  else if (_input.isReleased(BTN_OK))
  {
    uint16_t item_id = _menu->getCurrItemID();
    switch (item_id)
    {
      case ID_PROF_EDIT_NAME_ITEM:
        if (!_profile_edit_name.equals(STR_ITEM_TEMP_PROFILE))
          showProfileNameDialog();
        break;

      case ID_PROF_EDIT_VOLT_ITEM:
        showProfileMinVoltDialog();
        break;

      case ID_PROF_EDIT_CURR_ITEM:
        showProfileMaxCurrDialog();
        break;

      case ID_PROF_EDIT_FUN_ITEM:
        showProfileFunPwmDialog();
        break;

      default:
        log_e("Не реалізована функція");
        break;
    }
  }
}

void BattTestContext::showProfileNameDialog()
{
  _state_handler = &BattTestContext::handleProfileNameDialogState;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _profile_name_tb = new TextBox(ID_PROF_NAME_TB);
  layout->addWidget(_profile_name_tb);

  _profile_name_tb->setText(_profile_edit_name);
  _profile_name_tb->setHPadding(5);
  _profile_name_tb->setWidth(UI_WIDTH - 10);
  _profile_name_tb->setHeight(40);
  _profile_name_tb->setBackColor(COLOR_WHITE);
  _profile_name_tb->setTextColor(COLOR_BLACK);
  _profile_name_tb->setTextSize(2);
  _profile_name_tb->setPos(5, DISPLAY_CUTOUT);
  _profile_name_tb->setCornerRadius(3);

  _keyboard = WidgetCreator::getStandardEnKeyboard(ID_PROF_NAME_KB);
  layout->addWidget(_keyboard);

  _keyboard->setPos(0, _profile_name_tb->getBottomYPos() + 5);
}

void BattTestContext::handleProfileNameDialogState()
{
  if (_input.isPressed(BtnID::BTN_OK))
  {
    String tb_text = _profile_name_tb->getText();
    if (tb_text.isEmpty() || tb_text.equals(STR_ITEM_TEMP_PROFILE))
    {
      showToast(STR_ERR_INCORRECT_PROF_NAME);
    }
    else
    {
      _profile_edit_name = tb_text;
      showProfileEditTmpl();
    }
    return;
  }

  if (_input.isPressed(BtnID::BTN_BACK))
  {
    showProfileEditTmpl();
    return;
  }

  if (_input.isHolded(BtnID::BTN_UP))
    _keyboard->focusUp();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _keyboard->focusDown();
  else if (_input.isHolded(BtnID::BTN_RIGHT))
    _keyboard->focusRight();
  else if (_input.isHolded(BtnID::BTN_LEFT))
    _keyboard->focusLeft();
  else if (_input.isReleased(BtnID::BTN_OK))
    _profile_name_tb->addChars(_keyboard->getCurrBtnTxt().c_str());
  else if (_input.isReleased(BtnID::BTN_BACK))
    _profile_name_tb->removeLastChar();
}

void BattTestContext::showProfileMinVoltDialog()
{
  _state_handler = &BattTestContext::handleProfileMinVoltDialogState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* title_lbl = WidgetCreator::getWindowHeader(ID_PROF_EDIT_TITILE_LBL, STR_VOLTAGE_TITLE);
  layout->addWidget(title_lbl);

  _spinbox = new SpinBox(ID_PROF_SPINBOX);
  layout->addWidget(_spinbox);

  _spinbox->setFont(font_inr30);
  _spinbox->setBackColor(COLOR_WHITE);
  _spinbox->setTextColor(COLOR_BLACK);
  _spinbox->setCornerRadius(5);

  _spinbox->setType(SpinBox::TYPE_FLOAT);
  _spinbox->setStep(0.1f);
  _spinbox->setMin(MIN_TEST_VOLT);
  _spinbox->setMax(MAX_TEST_VOLT);
  _spinbox->setValue(_profile_edit_min_voltage);
  _spinbox->setWidth(130);
  _spinbox->setHeight(_spinbox->getHeight() + 5);
  _spinbox->setPos(getCenterX(_spinbox), getCenterY(_spinbox));

  addInstruction(STR_PROFILE_EDIT_SPIN_HINT, 4);
}

void BattTestContext::handleProfileMinVoltDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _profile_edit_min_voltage = _spinbox->getValue();
    showProfileEditTmpl();
    return;
  }

  if (_input.isReleased(BtnID::BTN_BACK))
  {
    showProfileEditTmpl();
    return;
  }

  if (_input.isHolded(BtnID::BTN_UP))
    _spinbox->up();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _spinbox->down();
}

void BattTestContext::showProfileMaxCurrDialog()
{
  _state_handler = &BattTestContext::handleProfileMaxCurrDialogState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* title_lbl = WidgetCreator::getWindowHeader(ID_PROF_EDIT_TITILE_LBL, STR_CURRENT_TITLE);
  layout->addWidget(title_lbl);

  _spinbox = new SpinBox(ID_PROF_SPINBOX);
  layout->addWidget(_spinbox);

  _spinbox->setFont(font_inr30);
  _spinbox->setBackColor(COLOR_WHITE);
  _spinbox->setTextColor(COLOR_BLACK);
  _spinbox->setCornerRadius(5);

  _spinbox->setType(SpinBox::TYPE_FLOAT);
  _spinbox->setStep(0.1f);
  _spinbox->setMin(0.0f);
  _spinbox->setMax(INA_MAX_CURR);
  _spinbox->setValue(_profile_edit_max_current);
  _spinbox->setWidth(130);
  _spinbox->setHeight(_spinbox->getHeight() + 5);
  _spinbox->setPos(getCenterX(_spinbox), getCenterY(_spinbox));

  addInstruction(STR_PROFILE_EDIT_SPIN_HINT, 4);
}

void BattTestContext::handleProfileMaxCurrDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _profile_edit_max_current = _spinbox->getValue();
    showProfileEditTmpl();
    return;
  }

  if (_input.isReleased(BtnID::BTN_BACK))
  {
    showProfileEditTmpl();
    return;
  }

  if (_input.isHolded(BtnID::BTN_UP))
    _spinbox->up();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _spinbox->down();
}

void BattTestContext::showProfileFunPwmDialog()
{
  _state_handler = &BattTestContext::handleProfileFunPwmDialogState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* title_lbl = WidgetCreator::getWindowHeader(ID_PROF_EDIT_TITILE_LBL, STR_FUN_TITLE);
  layout->addWidget(title_lbl);

  _spinbox = new SpinBox(ID_PROF_SPINBOX);
  layout->addWidget(_spinbox);

  _spinbox->setFont(font_inr30);
  _spinbox->setBackColor(COLOR_WHITE);
  _spinbox->setTextColor(COLOR_BLACK);
  _spinbox->setCornerRadius(5);

  _spinbox->setType(SpinBox::TYPE_INT);
  _spinbox->setStep(1.0f);
  _spinbox->setMin(0.0f);
  _spinbox->setMax(MAX_FUN_DUTY);
  _spinbox->setValue(_profile_edit_fun_pwm);
  _spinbox->setWidth(130);
  _spinbox->setHeight(_spinbox->getHeight() + 5);
  _spinbox->setPos(getCenterX(_spinbox), getCenterY(_spinbox));

  addInstruction(STR_PROFILE_EDIT_SPIN_HINT, 4);
}

void BattTestContext::handleProfileFunPwmDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _profile_edit_fun_pwm = static_cast<uint8_t>(_spinbox->getValue());
    showProfileEditTmpl();
    return;
  }

  if (_input.isReleased(BtnID::BTN_BACK))
  {
    showProfileEditTmpl();
    return;
  }

  if (_input.isHolded(BtnID::BTN_UP))
    _spinbox->up();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _spinbox->down();
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showManualTestTmpl()
{
  _state_handler = &BattTestContext::handleManualTestState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _voltage_lbl = new Label(ID_VOLTAGE_LBL);
  layout->addWidget(_voltage_lbl);
  _voltage_lbl->setText(STR_EMPTY_BAT);
  _voltage_lbl->setFont(font_inr30);
  _voltage_lbl->setWidth(UI_WIDTH / 2 - 5);
  _voltage_lbl->setHeight((UI_HEIGHT - 16) / 4);
  _voltage_lbl->setBackColor(COLOR_RED);
  _voltage_lbl->setAlign(IWidget::ALIGN_CENTER);
  _voltage_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _voltage_lbl->setCornerRadius(10);

  _current_lbl = _voltage_lbl->clone(ID_CURRENT_LBL);
  layout->addWidget(_current_lbl);
  _current_lbl->setPos(0, _voltage_lbl->getBottomYPos() + 5);
  _current_lbl->setBackColor(COLOR_BLUE);

  _power_lbl = _voltage_lbl->clone(ID_POWER_LBL);
  layout->addWidget(_power_lbl);
  _power_lbl->setPos(0, _current_lbl->getBottomYPos() + 5);
  _power_lbl->setBackColor(COLOR_GREEN);

  _fun_pwm_lbl = _voltage_lbl->clone(ID_FUN_PWM_LBL);
  layout->addWidget(_fun_pwm_lbl);
  _fun_pwm_lbl->setFont(font_10x20);
  _fun_pwm_lbl->setPos(_voltage_lbl->getRightXPos() + 5, _voltage_lbl->getYPos());
  _fun_pwm_lbl->setBackColor(COLOR_LIGHTGREY);

  _opa_pwm_lbl = _fun_pwm_lbl->clone(ID_OPA_PWM_LBL);
  layout->addWidget(_opa_pwm_lbl);
  _opa_pwm_lbl->setPos(_current_lbl->getRightXPos() + 5, _current_lbl->getYPos());

  _test_time_lbl = _fun_pwm_lbl->clone(ID_TEST_TIME_LBL);
  layout->addWidget(_test_time_lbl);
  _test_time_lbl->setPos(_power_lbl->getRightXPos() + 5, _power_lbl->getYPos());

  addInstruction(STR_MANUAL_TEST_HINT, 5);

  turnOnFun(_fun_duty);
  turnOnLoad(_opa_duty);

  updateFunPwmLbl();
  updateOpaPwmLbl();

  _test_start_ts = millis();
  updateTestTime();
}

void BattTestContext::handleManualTestState()
{
  updateReadings();
  updateTestTime();

  float pg_voltage = readPG();
  if (!checkVoltageDiff(_ina_bus_voltage, pg_voltage) || pg_voltage < MIN_PG_VOLT)
  {
    turnOffLoad();
    turnOffFun();
    return;
  }

  if (_input.isPressed(BTN_BACK))
  {
    _readings_upd_ts = 0;
    turnOffLoad();
    turnOffFun();
    showMainTmpl();
    return;
  }

  if (_input.isHolded(BTN_RIGHT))
  {
    incOpaPwm();
    updateOpaPwmLbl();
  }
  else if (_input.isHolded(BTN_LEFT))
  {
    decOpaPwm();
    updateOpaPwmLbl();
  }
  else if (_input.isHolded(BTN_UP))
  {
    incFunSpeed();
    updateFunPwmLbl();
  }
  else if (_input.isHolded(BTN_DOWN))
  {
    decFunSpeed();
    updateFunPwmLbl();
  }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showTestTmpl()
{
  _state_handler = &BattTestContext::handleTestState;

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _voltage_lbl = new Label(ID_VOLTAGE_LBL);
  layout->addWidget(_voltage_lbl);
  _voltage_lbl->setText(STR_EMPTY_BAT);
  _voltage_lbl->setFont(font_inr30);
  _voltage_lbl->setWidth(UI_WIDTH / 2 - 5);
  _voltage_lbl->setHeight((UI_HEIGHT - 16) / 4);
  _voltage_lbl->setBackColor(COLOR_RED);
  _voltage_lbl->setAlign(IWidget::ALIGN_CENTER);
  _voltage_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _voltage_lbl->setCornerRadius(10);

  _current_lbl = _voltage_lbl->clone(ID_CURRENT_LBL);
  layout->addWidget(_current_lbl);
  _current_lbl->setPos(0, _voltage_lbl->getBottomYPos() + 5);
  _current_lbl->setBackColor(COLOR_BLUE);

  _power_lbl = _voltage_lbl->clone(ID_POWER_LBL);
  layout->addWidget(_power_lbl);
  _power_lbl->setPos(0, _current_lbl->getBottomYPos() + 5);
  _power_lbl->setBackColor(COLOR_GREEN);

  Label* min_voltage_lbl = _voltage_lbl->clone(ID_MIN_VOLTAGE_LBL);
  layout->addWidget(min_voltage_lbl);
  min_voltage_lbl->setFont(font_10x20);
  min_voltage_lbl->setPos(_voltage_lbl->getRightXPos() + 5, _voltage_lbl->getYPos());
  min_voltage_lbl->setBackColor(COLOR_LIGHTGREY);

  String min_volt_str = "Min: ";
  min_volt_str += _min_test_voltage;
  min_volt_str += "V";
  min_voltage_lbl->setText(min_volt_str);

  Label* max_current_lbl = min_voltage_lbl->clone(ID_MAX_CURRENT_LBL);
  layout->addWidget(max_current_lbl);
  max_current_lbl->setPos(min_voltage_lbl->getXPos(), min_voltage_lbl->getBottomYPos() + 5);

  String max_cur_str = "Max: ";
  max_cur_str += _max_current;
  max_cur_str += "A";
  max_current_lbl->setText(max_cur_str);

  _capacity_lbl = min_voltage_lbl->clone(ID_CAPACITY_LBL);
  layout->addWidget(_capacity_lbl);
  _capacity_lbl->setPos(max_current_lbl->getXPos(), max_current_lbl->getBottomYPos() + 5);

  _test_time_lbl = min_voltage_lbl->clone(ID_TEST_TIME_LBL);
  layout->addWidget(_test_time_lbl);
  _test_time_lbl->setPos(_capacity_lbl->getXPos(), _capacity_lbl->getBottomYPos() + 5);

  _test_start_ts = millis();
  _is_working = true;
  updateTestTime();
  turnOnFun(_fun_duty);
}

void BattTestContext::handleTestState()
{
  if (_input.isPressed(BTN_BACK))
  {
    _readings_upd_ts = 0;
    turnOffLoad();
    turnOffFun();
    showMainTmpl();
    return;
  }

  if (_input.isHolded(BTN_UP))
    incFunSpeed();
  else if (_input.isHolded(BTN_DOWN))
    decFunSpeed();

  updateReadings(true);

  if (!_is_working)
    return;

  updateTestTime();

  float pg_voltage = readPG();
  if (!checkVoltageDiff(_ina_bus_voltage, pg_voltage) || pg_voltage < MIN_PG_VOLT)
  {
    turnOffLoad();
    turnOffFun();
    return;
  }

  if (_ina_bus_voltage <= _min_test_voltage)
  {
    turnOffLoad();
    turnOffFun();
    return;
  }

  adjustOpaPWM();
}

void BattTestContext::adjustOpaPWM()
{
  if (_test_current > _max_current + 0.01)
  {
    decOpaPwm();
  }
  else if (_test_current < _max_current - 0.01)
  {
    incOpaPwm();
  }
}

// -----------------------------------------------------------------------------------------------------------------------

float BattTestContext::readPG()
{
  return readPinVoltage(PIN_PG, SAMPLES_NUM, R_DIV);
}

void BattTestContext::incFunSpeed()
{
  if (_fun_duty == 0)
    return;

  --_fun_duty;
  ledcWrite(PIN_FUN_PWM, _fun_duty);
}

void BattTestContext::decFunSpeed()
{
  if (_fun_duty == MAX_FUN_DUTY)
    return;

  ++_fun_duty;
  ledcWrite(PIN_FUN_PWM, _fun_duty);
}

void BattTestContext::incOpaPwm()
{
  if (_opa_duty == MAX_OPA_DUTY)
    return;

  if (_opa_duty < 1000)  // З низькою напругою немає сенсу точно налаштовувати
    _opa_duty += 40;
  else if (_opa_duty < 1200)
    _opa_duty += 20;
  else
    ++_opa_duty;

  ledcWrite(PIN_OPA_PWM, _opa_duty);
}

void BattTestContext::decOpaPwm()
{
  if (_opa_duty == 0)
    return;

  --_opa_duty;
  ledcWrite(PIN_OPA_PWM, _opa_duty);
}

bool BattTestContext::checkVoltageDiff(float bus_voltage, float pg_voltage)
{
  float volt_diff = std::abs(bus_voltage - pg_voltage);
  if (volt_diff > MAX_VOLT_DIFF)
  {
    log_e("Показники напруги INA219 та PG відрізняються на %f В", volt_diff);
    showErrLabelTmpl(STR_ERR_VOLT_DIFF);
    return false;
  }

  return true;
}

void BattTestContext::updateReadings(bool update_capacity)
{
  if (millis() - _readings_upd_ts < UPD_READINGS_DELAY)
    return;

  float voltage = _ina.getBusVoltage();
  _ina_bus_voltage = voltage;  // Зберігаємо для перевірки стану INA219
  String volt_str = String(voltage);
  volt_str += "V";
  _voltage_lbl->setText(volt_str);

  float current = std::abs(_ina.getCurrent());
  _test_current = current;  // Зберігаємо для тимчасового/нового профілю
  String current_str = String(current);
  current_str += "A";
  _current_lbl->setText(current_str);

  float power = voltage * current;
  String power_str = String(power);
  power_str += "W";
  _power_lbl->setText(power_str);

  if (update_capacity)
    updateCapacity(current);

  _readings_upd_ts = millis();
}

void BattTestContext::updateCapacity(float current)
{
  if (_readings_upd_ts == 0)
  {
    _capacity_mah = 0;
    _prev_current = current;
    return;
  }

  // Метод трапецій(середнє арифметичне)
  const double average_current = 0.5 * (static_cast<double>(_prev_current) + current);
  _prev_current = current;

  const uint32_t elapsed_ms = millis() - _readings_upd_ts;
  _capacity_mah += fabs(average_current) * elapsed_ms / 3600.0;

  String capacity_str = String(_capacity_mah);
  capacity_str += "mAh";
  _capacity_lbl->setText(capacity_str);
}

void BattTestContext::updateTestTime()
{
  if (millis() - _upd_test_time_ts < 1000)
    return;

  const unsigned int total_time = millis() - _test_start_ts;
  String test_time_str;

  uint32_t minutes = floor(static_cast<float>(total_time) / 60000);
  if (minutes < 100)
    test_time_str += "0";
  if (minutes < 10)
    test_time_str += "0";
  test_time_str += String(minutes);

  test_time_str += ":";

  uint32_t sec = static_cast<float>(total_time - minutes * 60000) / 1000;

  if (sec < 10)
    test_time_str += "0";
  test_time_str += String(sec);

  _test_time_lbl->setText(test_time_str);

  _upd_test_time_ts = millis();
}

void BattTestContext::updateFunPwmLbl()
{
  String fun_pwm_str{STR_ITEM_PROFILE_FUN};
  fun_pwm_str += _fun_duty;
  _fun_pwm_lbl->setText(fun_pwm_str);
}

void BattTestContext::updateOpaPwmLbl()
{
  String opa_pwm_str{"OPA ШІМ: "};
  opa_pwm_str += _opa_duty;
  _opa_pwm_lbl->setText(opa_pwm_str);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::turnOnFun(uint8_t duty)
{
  ledcWrite(PIN_FUN_PWM, duty);
}

void BattTestContext::turnOffFun()
{
  ledcWrite(PIN_FUN_PWM, UINT8_MAX);
}

void BattTestContext::turnOnLoad(uint16_t pwm_duty)
{
  if (pwm_duty > MAX_OPA_DUTY)
  {
    log_e("Некоректне значення opa_duty: %u", pwm_duty);
    pwm_duty = 0;
  }

  ledcWrite(PIN_OPA_PWM, pwm_duty);
}

void BattTestContext::turnOffLoad()
{
  _is_working = false;
  _opa_duty = 0;
  
  ledcWrite(PIN_OPA_PWM, 0);
}
// -----------------------------------------------------------------------------------------------------------------------
