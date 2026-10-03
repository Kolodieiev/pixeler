#include "BattTestContext.h"

#include <esp32-hal-ledc.h>
#include <math.h>

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

static const char STR_ITEM_SEL_PROFILE[] = "Обрати профіль тестування";
static const char STR_ITEM_MANUAL_TESTING[] = "Ручне тестування";

static const char STR_ITEM_TEMP_PROFILE[] = "Тимчасовий профіль";
static const char STR_ITEM_RUN_PROFILE[] = "Запустити тест";
static const char STR_ITEM_NEW_PROFILE[] = "Новий профіль";

static const float INA_MAX_CURR = 3.2f;  // Максимальний струм можливий для шунта INA219
static const float INA_SHUNT_R = 0.1f;   // Значення опору шунта INA219

static const float MIN_WARNING_CURRENT = 0.035f;  // Мінімальний струм спокою, при якому модуль вважається пошкодженим
static const float MIN_PG_VOLT = 0.4f;            // Мінімальна напргуа на піні PG при якому акум вважається підключеним
static const float MIN_TEST_VOLT = 0.4f;          // Мінімальна можлива напруга тестування акума
static const float MAX_VOLT_DIFF = 0.2f;          // Максимальне допустиме значення розбіжності напруг на модулі та PG

static const float R_DIV = 0.0917f;  // Точний коефіцієнт подільника на піні PG

static const float TEMP_PROF_MIN_VOLT = 3.0f;  // Мінімальна напруга тимчасового профіля

static const unsigned long UPD_READINGS_DELAY = 500LU;  // Затримка між оновленням показників в UI

static const uint32_t FUN_PWM_FREQ = 40000;
static const uint32_t OPA_PWM_FREQ = 70000;

static const uint16_t SAMPLES_NUM = 128;
static const uint16_t MAX_OPA_DUTY = 1024;

static const uint8_t INA_ADDR = 0x40;  // Адреса модуля INA219

static const uint8_t PWM_FUN_RES = 8;
static const uint8_t PWM_OPA_RES = 10;

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

  Label* instruction_lbl = new Label(ID_INSTRUCTION_LBL);
  layout->addWidget(instruction_lbl);

  instruction_lbl->setText(STR_MAIN_HINT);

  instruction_lbl->setFont(font_unifont);
  instruction_lbl->setWidth(UI_WIDTH - DISPLAY_CUTOUT * 2);
  instruction_lbl->setMultiline(true);
  instruction_lbl->setHeight(instruction_lbl->getCharHgt() * 3);
  instruction_lbl->setPos(DISPLAY_CUTOUT, _voltage_lbl->getBottomYPos() + 5);
  instruction_lbl->setBackColor(layout->getBackColor());
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

  _context_menu = WidgetCreator::getContextMenu(ID_MAIN_CONTEXT);
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
  getLayout()->delWidgetByID(ID_MAIN_CONTEXT);
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

  _prof_hint_lbl = new Label(ID_PROF_HINT_LABEL);
  layout->addWidget(_prof_hint_lbl);
  _prof_hint_lbl->setFont(font_8x13);
  _prof_hint_lbl->setWidth(UI_WIDTH - DISPLAY_CUTOUT * 2);
  _prof_hint_lbl->setPos(DISPLAY_CUTOUT, UI_HEIGHT - _prof_hint_lbl->getHeight());
  _prof_hint_lbl->setAlign(IWidget::ALIGN_CENTER);
  _prof_hint_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _prof_hint_lbl->setBackColor(layout->getBackColor());

  updateProfileHint();

  // Тимчасовий профіль
  MenuItem* temp_item = WidgetCreator::getMenuItem(ID_ITEM_TEMP_PROFILE);
  _menu->addItem(temp_item);

  Label* temp_lbl = WidgetCreator::getItemLabel(STR_ITEM_TEMP_PROFILE, font_10x20);
  temp_item->setLabel(temp_lbl);

  if (!_fs.isMounted())
    return;

  // TODO завантажити інші профілі, якщо карта примонтована
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

  if (item_id == ID_ITEM_TEMP_PROFILE)
  {
    _min_test_voltage = TEMP_PROF_MIN_VOLT;
  }
  else
  {
    // TODO завантажити параметри конкретного профілю з карти памяті
    // Додати структуру для збереження параметрів групою в файлі
  }

  String hint_str = String(_min_test_voltage);
  hint_str += "V  ";
  hint_str += _test_current;
  hint_str += "A  ";
  hint_str += "Fun: ";
  hint_str += _fun_duty;

  _prof_hint_lbl->setText(hint_str);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::showProfileContextMenu()
{
  _state_handler = &BattTestContext::handleProfileContextMenuState;

  _context_menu = WidgetCreator::getContextMenu(ID_PROFILE_CONTEXT);
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
  getLayout()->delWidgetByID(ID_PROFILE_CONTEXT);
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
      case ID_ITEM_RUN_PROFILE:  // TODO
        break;

      case ID_ITEM_DELETE_PROFILE:  // TODO
        break;

      case ID_ITEM_EDIT_PROFILE:
        showProfileEditDialog(_menu->getCurrItemText(), _min_test_voltage, _test_current, _fun_duty);
        break;

      case ID_ITEM_NEW_PROFILE:
        showProfileEditDialog(STR_ITEM_NEW_PROFILE, _min_test_voltage, _test_current, _fun_duty);
        break;

      default:
        log_e("Не реалізована функція");
        break;
    }
  }
}

void BattTestContext::showProfileEditDialog(const String& profile_name, float min_voltage, float test_current, uint8_t fun_pwm)  // TODO
{
  _state_handler = &BattTestContext::handleEditProfileState;
}

void BattTestContext::handleEditProfileState()  // TODO
{
  // Перемикатися між полями вводу. Якщо профіль не тимчасовий, Додати шаблон вводу імені.
  // Перевіряти щоб введене імя не було "Тимчасовий профіль"
  // зберігати профіль
  // PRESS_ОК - відкриває showProfileEditDialog з введеним іменем, PRESS_BACK - з тимчасово збереженим старим іменем
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
  _voltage_lbl->setHeight(_voltage_lbl->getHeight() + 6);
  _voltage_lbl->setBackColor(COLOR_RED);
  _voltage_lbl->setAlign(IWidget::ALIGN_CENTER);
  _voltage_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _voltage_lbl->setCornerRadius(5);

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

  Label* instruction_lbl = new Label(ID_INSTRUCTION_LBL);
  layout->addWidget(instruction_lbl);
  instruction_lbl->setText(STR_MANUAL_TEST_HINT);
  instruction_lbl->setFont(font_5x7);
  instruction_lbl->setWidth(UI_WIDTH - DISPLAY_CUTOUT * 2);
  instruction_lbl->setMultiline(true);
  instruction_lbl->setHeight(instruction_lbl->getCharHgt() * 5);
  instruction_lbl->setPos(DISPLAY_CUTOUT, _power_lbl->getBottomYPos() + 5);
  instruction_lbl->setBackColor(layout->getBackColor());

  turnOnFun();
  updateFunPwmLbl();
  updateOpaPwmLbl();
}

void BattTestContext::handleManualTestState()
{
  if (readPG() < MIN_PG_VOLT || _input.isPressed(BTN_BACK))
  {
    turnOffLoad();
    turnOffFun();
    showMainTmpl();
    return;
  }

  updateReadings();

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

void BattTestContext::showTestTmpl()  // TODO
{
  _state_handler = &BattTestContext::handleTestState;
}

void BattTestContext::handleTestState()  // TODO
{
  //  коригувати шім під струм та перевіряти умови
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
  if (_fun_duty == 100)
    return;

  ++_fun_duty;
  ledcWrite(PIN_FUN_PWM, _fun_duty);
}

void BattTestContext::incOpaPwm()
{
  if (_opa_duty == MAX_OPA_DUTY)
    return;

  if (_opa_duty < 200)  // Менше 2-3 В немає сенсу точно налаштовувати
    _opa_duty += 40;
  else if (_opa_duty < 300)
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

void BattTestContext::updateReadings()
{
  if (millis() - _readings_upd_ts < UPD_READINGS_DELAY)
    return;

  float voltage = _ina.getBusVoltage();
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

  _readings_upd_ts = millis();
}

void BattTestContext::updateFunPwmLbl()
{
  String fun_pwm_str{"Fun PWM: "};
  fun_pwm_str += _fun_duty;
  _fun_pwm_lbl->setText(fun_pwm_str);
}

void BattTestContext::updateOpaPwmLbl()
{
  String opa_pwm_str{"OPA PWM: "};
  opa_pwm_str += _opa_duty;
  _opa_pwm_lbl->setText(opa_pwm_str);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattTestContext::turnOnFun()
{
  ledcWrite(PIN_FUN_PWM, 0);
}

void BattTestContext::turnOffFun()
{
  ledcWrite(PIN_FUN_PWM, UINT8_MAX);
}

void BattTestContext::turnOnLoad(uint16_t pwm_duty)
{
  if (pwm_duty > MAX_OPA_DUTY)
    pwm_duty = MAX_OPA_DUTY;

  _opa_duty = pwm_duty;
  ledcWrite(PIN_OPA_PWM, _opa_duty);
}

void BattTestContext::turnOffLoad()
{
  ledcWrite(PIN_OPA_PWM, 0);
}
// -----------------------------------------------------------------------------------------------------------------------
