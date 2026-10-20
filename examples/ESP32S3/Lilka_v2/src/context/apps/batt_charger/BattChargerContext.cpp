#include "BattChargerContext.h"

#include "../../WidgetCreator.h"
#include "context/apps/AppsContext.h"
#include "manager/SettingsManager.h"

static const char STR_ERR_ADS[] = "ADS не відповідає";
static const char STR_ERR_VOLT_DIFF[] = "Напруга на датчиках відрізняється";

static const char STR_ITEM_RUN_PROFILE[] = "Розпочати заряджання";

static const char STR_ITEM_PROFILE_NAME[] = "Ім'я: ";
static const char STR_ITEM_PROFILE_VOLT[] = "Макс. V: ";
static const char STR_ITEM_PROFILE_CURR[] = "Макс. C: ";

static const char STR_VOLTAGE_TITLE[] = "Максимальна напруга";
static const char STR_CURRENT_TITLE[] = "Максимальний струм";

static const char STR_APP_DIR[] = "batt_charger";

static const char STR_BAT_TYPE_HINT_TITLE[] = "Тип: ";
static const char STR_BAT_TYPE_HINT_LIP[] = "Li";
static const char STR_BAT_TYPE_HINT_HVLIP[] = "HVLi";
static const char STR_BAT_TYPE_HINT_LIFEPO[] = "FePO4";

static const char STR_BAT_TYPE_LIP[] = "Літій-іон/полімер";
static const char STR_BAT_TYPE_HVLIP[] = "Літій 4.4В";
static const char STR_BAT_TYPE_LIFEPO[] = "Літій-залізо-фосфат";

static const char STR_PROFILE_EDIT_HINT[] =
    "Press A -> Зберегти\n"
    "Press B -> Вихід";

static const char STR_SELECT_ITEM_HINT[] =
    "Click A -> Зберегти\n"
    "Click B -> Вихід";

static const char STR_PROFILE_EDIT_SPIN_HINT[] =
    "Hold UP/DOWN -> Змінити значення\n"
    "Click A -> Зафіксувати\n"
    "Click B -> Вихід";

static const float INA_MAX_CURR = 3.2f;           // Максимальний струм можливий для шунта INA219
static const float INA_SHUNT_R = 0.1f;            // Значення опору шунта INA219
static const float MAX_CUR_DEV = 0.02f;           // Максимальне можливе відхилення від заданого струму в режимі тестування
static const float MAX_VOLT_DIFF = 0.05f;         // Максимальне допустиме значення розбіжності напруги акума на модулях-датчиках
static const float TEMP_PROF_MAX_VOLT = 4.2f;     // Максимальна напруга тимчасового профіля
static const float TEMP_PROF_MAX_CURRENT = 0.1f;  // Максимальний струм тимчасового профіля
static const float MIN_CHARG_VOLT = 1.5f;
static const float MAX_CHARG_CURRENT = 2.0f;

static const float MAX_VOLT_LIP = 4.2f;
static const float MAX_VOLT_HVLIP = 4.4f;
static const float MAX_VOLT_LIFEPO = 3.6f;

static const unsigned long UPD_READINGS_DELAY = 100LU;  // Затримка між оновленням показників в UI
static const unsigned long WDT_CHECK_DELAY = 2000UL;

static const uint32_t PWM_VOUT_FREQ = 17000;

static const uint16_t MAX_VOUT_PWM_DUTY = 4096;

static const uint8_t INA_ADDR = 0x40;  // Адреса модуля INA219

static const uint8_t PIN_DC_IN_EN = 13;
static const uint8_t PIN_DC_OUT_EN = 14;
static const uint8_t PIN_VOUT_PWM = 12;
static const uint8_t PWM_VOUT_RES = 12;

bool BattChargerContext::loop()
{
  return true;
}

void BattChargerContext::update()
{
  (this->*_state_handler)();
}

BattChargerContext::BattChargerContext() : _ina{INA_ADDR}, _profile{.max_voltage = TEMP_PROF_MAX_VOLT, .max_current = TEMP_PROF_MAX_CURRENT}
{
  pinMode(PIN_DC_IN_EN, OUTPUT);
  pinMode(PIN_DC_OUT_EN, OUTPUT);

  turnOffDcIn();

  ledcAttach(PIN_VOUT_PWM, PWM_VOUT_FREQ, PWM_VOUT_RES);
  ledcWrite(PIN_VOUT_PWM, 0);

  _i2c.begin();

  if (!_ina.begin() || !_i2c.hasConnect(INA_ADDR))
  {
    log_e("Модуль INA219 не відповідає за вказаною адресою: 0x%02X", INA_ADDR);
    showErrLabelTmpl(STR_ERR_INA219);

    return;
  }

  if (!_ads.begin() || !_i2c.hasConnect(ADS1X15_ADDRESS))
  {
    log_e("Модуль ADS не відповідає за вказаною адресою: 0x%02X", ADS1X15_ADDRESS);
    showErrLabelTmpl(STR_ERR_ADS);

    return;
  }

  _ina.setMaxCurrentShunt(INA_MAX_CURR, INA_SHUNT_R);
  _ina.setShuntSamples(5);
  _ina.setBusSamples(5);

  updateVoltages();
  if (!checkOutVoltageDiff())
    return;

  showMainTmpl();
}

BattChargerContext::~BattChargerContext()
{
  ledcWrite(PIN_VOUT_PWM, 0);
  ledcDetach(PIN_VOUT_PWM);

  _i2c.end();
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showErrLabelTmpl(const String& msg_str)
{
  _state_handler = &BattChargerContext::handleErrState;

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

void BattChargerContext::handleErrState()
{
  if (_input.isReleased(BTN_BACK))
    openContext(new AppsContext());
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::addInstruction(const String& text, uint8_t line_num)
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

void BattChargerContext::showMainTmpl()
{
  _state_handler = &BattChargerContext::handleMainState;

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

  Label* temp_lbl = WidgetCreator::getItemLabel(STR_TEMP_PROFILE, font_10x20);
  temp_item->setLabel(temp_lbl);

  updateProfileHint();

  if (!_fs.isMounted())
    return;

  String dir_path = SettingsManager::getSettingsDirPath(STR_APP_DIR);
  if (dir_path.isEmpty())
    return;

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

void BattChargerContext::handleMainState()
{
  if (_input.isReleased(BTN_BACK))
  {
    openContext(new AppsContext());
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

void BattChargerContext::updateProfileHint()
{
  uint16_t item_id = _menu->getCurrItemID();

  float hint_voltage = 999.0f;
  float hint_current = 0.0f;
  BatType batt_type = BatType::BATT_TYPE_LIP;

  if (item_id == ID_ITEM_TEMP_PROFILE)
  {
    hint_voltage = _profile.max_voltage;
    hint_current = _profile.max_current;
  }
  else
  {
    ProfileSetup prof_setup;
    if (SettingsManager::load(&prof_setup, sizeof(prof_setup), _menu->getCurrItemText(), STR_APP_DIR))
    {
      hint_voltage = prof_setup.max_voltage;
      hint_current = prof_setup.max_current;
      batt_type = prof_setup.batt_type;
    }
  }

  String hint_str = String(hint_voltage);
  hint_str += "V  ";
  hint_str += hint_current;
  hint_str += "A  ";
  hint_str += "Type: ";
  hint_str += getNameByBattType(batt_type);

  _prof_hint_lbl->setText(hint_str);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showProfileContextMenu()
{
  _state_handler = &BattChargerContext::handleProfileContextMenuState;

  _context_menu = WidgetCreator::getContextMenu(ID_C_MENU);
  getLayout()->addWidget(_context_menu);

  // Запустити заряджання
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

    Label* new_profile_lbl = WidgetCreator::getItemLabel(STR_NEW_PROFILE, font_unifont);
    new_profile_item->setLabel(new_profile_lbl);
    new_profile_lbl->setHPadding(1);
  }

  _context_menu->setHeight(_context_menu->getItemHeight() * _context_menu->getSize() + 4);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight() - DISPLAY_CUTOUT);
}

void BattChargerContext::hideProfileContextMenu()
{
  getLayout()->delWidgetByID(ID_C_MENU);
  _state_handler = &BattChargerContext::handleMainState;
}

void BattChargerContext::handleProfileContextMenuState()
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
        if (!profile_name.equals(STR_TEMP_PROFILE))
        {
          ProfileSetup prof_setup;
          bool result = SettingsManager::load(&prof_setup, sizeof(prof_setup), _menu->getCurrItemText(), STR_APP_DIR);
          if (!result)
          {
            showToast(STR_FAIL);
            return;
          }
          else
          {
            _profile = prof_setup;
          }
        }

        showChargeTmpl();
      }

      break;

      case ID_ITEM_DELETE_PROFILE:
        if (!SettingsManager::remove(_menu->getCurrItemText(), STR_APP_DIR))
        {
          showToast(STR_FAIL);
        }
        else
        {
          showMainTmpl();
          showToast(STR_SUCCESS);
        }
        break;

      case ID_ITEM_EDIT_PROFILE:
      {
        _profile_edit_name = _menu->getCurrItemText();

        ProfileSetup prof_setup;
        if (!_profile_edit_name.equals(STR_TEMP_PROFILE))
        {
          if (!SettingsManager::load(&prof_setup, sizeof(prof_setup), _profile_edit_name, STR_APP_DIR))
            showToast(STR_FAIL);
          else
            _profile_edit = prof_setup;
        }
        else
        {
          _profile_edit.max_voltage = TEMP_PROF_MAX_VOLT;
          _profile_edit.max_current = TEMP_PROF_MAX_CURRENT;
          _profile_edit.batt_type = BatType::BATT_TYPE_LIP;
        }

        showProfileEditTmpl();
      }
      break;

      case ID_ITEM_NEW_PROFILE:
        _profile_edit_name = STR_NEW_PROFILE;
        _profile_edit.max_voltage = TEMP_PROF_MAX_VOLT;
        _profile_edit.max_current = TEMP_PROF_MAX_CURRENT;
        _profile_edit.batt_type = BatType::BATT_TYPE_LIP;

        showProfileEditTmpl();
        break;

      default:
        log_e("%s", STR_ERR_UNIMPLEMENTED_FUNC);
        break;
    }
  }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showProfileEditTmpl()
{
  _state_handler = &BattChargerContext::handleProfileEditState;

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
  MenuItem* prof_name_item = WidgetCreator::getMenuItem(ID_ITEM_PROF_EDIT_NAME);
  _menu->addItem(prof_name_item);

  String prof_name_text = STR_ITEM_PROFILE_NAME;
  prof_name_text += _profile_edit_name;
  Label* prof_name_lbl = WidgetCreator::getItemLabel(prof_name_text, font_10x20);
  prof_name_item->setLabel(prof_name_lbl);

  // Тип акумулятора
  MenuItem* prof_batt_type_item = WidgetCreator::getMenuItem(ID_ITEM_PROF_EDIT_BATT_TYPE);
  _menu->addItem(prof_batt_type_item);

  String prof_batt_type_text = STR_BAT_TYPE_HINT_TITLE;
  prof_batt_type_text += getNameByBattType(_profile_edit.batt_type);

  Label* prof_batt_type_lbl = WidgetCreator::getItemLabel(prof_batt_type_text, font_10x20);
  prof_batt_type_item->setLabel(prof_batt_type_lbl);

  // Максимальна напруга
  MenuItem* prof_volt_item = WidgetCreator::getMenuItem(ID_ITEM_PROF_EDIT_VOLT);
  _menu->addItem(prof_volt_item);

  String prof_volt_text = STR_ITEM_PROFILE_VOLT;
  prof_volt_text += _profile_edit.max_voltage;
  Label* prof_volt_lbl = WidgetCreator::getItemLabel(prof_volt_text, font_10x20);
  prof_volt_item->setLabel(prof_volt_lbl);

  // Максимальний струм
  MenuItem* prof_curr_item = WidgetCreator::getMenuItem(ID_ITEM_PROF_EDIT_CURR);
  _menu->addItem(prof_curr_item);

  String prof_curr_text = STR_ITEM_PROFILE_CURR;
  prof_curr_text += _profile_edit.max_current;
  Label* prof_curr_lbl = WidgetCreator::getItemLabel(prof_curr_text, font_10x20);
  prof_curr_item->setLabel(prof_curr_lbl);
}

void BattChargerContext::handleProfileEditState()
{
  if (_input.isPressed(BTN_BACK))
  {
    showMainTmpl();
    return;
  }

  if (_input.isPressed(BTN_OK))
  {
    if (_profile_edit_name.equals(STR_TEMP_PROFILE))
    {
      _profile = _profile_edit;
      showToast(STR_SUCCESS);
    }
    else if (!SettingsManager::save(&_profile_edit, sizeof(_profile_edit), _profile_edit_name, STR_APP_DIR))
      showToast(STR_FAIL);
    else
      showToast(STR_SUCCESS);

    showMainTmpl();
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
      case ID_ITEM_PROF_EDIT_NAME:
        if (!_profile_edit_name.equals(STR_TEMP_PROFILE))
          showProfileNameDialog();
        break;

      case ID_ITEM_PROF_EDIT_VOLT:
        showProfileMaxVoltDialog();
        break;

      case ID_ITEM_PROF_EDIT_CURR:
        showProfileMaxCurrDialog();
        break;

      case ID_ITEM_PROF_EDIT_BATT_TYPE:
        showProfileBatTypeDialog();
        break;

      default:
        log_e("%s", STR_ERR_UNIMPLEMENTED_FUNC);
        break;
    }
  }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showProfileNameDialog()
{
  _state_handler = &BattChargerContext::handleProfileNameDialogState;

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

void BattChargerContext::handleProfileNameDialogState()
{
  if (_input.isPressed(BtnID::BTN_OK))
  {
    String tb_text = _profile_name_tb->getText();
    if (tb_text.isEmpty() || tb_text.equals(STR_TEMP_PROFILE))
    {
      showToast(STR_ERR_INCORRECT_NAME);
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

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showProfileMaxVoltDialog()
{
  _state_handler = &BattChargerContext::handleProfileMaxVoltDialogState;

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
  _spinbox->setMin(MIN_CHARG_VOLT);

  switch (_profile_edit.batt_type)
  {
    case BatType::BATT_TYPE_LIP:
      _spinbox->setMax(MAX_VOLT_LIP);
      break;

    case BatType::BATT_TYPE_HVLIP:
      _spinbox->setMax(MAX_VOLT_HVLIP);
      break;

    case BatType::BATT_TYPE_LIFEPO:
      _spinbox->setMax(MAX_VOLT_LIFEPO);
      break;

    default:
      _spinbox->setMax(0);
      break;
  }

  _spinbox->setValue(_profile_edit.max_voltage);
  _spinbox->setWidth(130);
  _spinbox->setHeight(_spinbox->getHeight() + 5);
  _spinbox->setPos(getCenterX(_spinbox), getCenterY(_spinbox));

  addInstruction(STR_PROFILE_EDIT_SPIN_HINT, 4);
}

void BattChargerContext::handleProfileMaxVoltDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _profile_edit.max_voltage = _spinbox->getValue();
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

void BattChargerContext::showProfileMaxCurrDialog()
{
  _state_handler = &BattChargerContext::handleProfileMaxCurrDialogState;

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
  _spinbox->setStep(0.01f);
  _spinbox->setMin(0.01f);
  _spinbox->setMax(MAX_CHARG_CURRENT);
  _spinbox->setValue(_profile_edit.max_current);
  _spinbox->setWidth(130);
  _spinbox->setHeight(_spinbox->getHeight() + 5);
  _spinbox->setPos(getCenterX(_spinbox), getCenterY(_spinbox));

  addInstruction(STR_PROFILE_EDIT_SPIN_HINT, 4);
}

void BattChargerContext::handleProfileMaxCurrDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _profile_edit.max_current = _spinbox->getValue();
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

void BattChargerContext::showProfileBatTypeDialog()
{
  _state_handler = &BattChargerContext::handleProfileBatTypeDialogState;

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

  addInstruction(STR_SELECT_ITEM_HINT, 3);

  // Літій
  MenuItem* item_lip_type = WidgetCreator::getMenuItem(ID_ITEM_LIP);
  _menu->addItem(item_lip_type);

  Label* lip_type_lbl = WidgetCreator::getItemLabel(STR_BAT_TYPE_LIP, font_10x20);
  item_lip_type->setLabel(lip_type_lbl);

  // Літій підвищеної напруги
  MenuItem* item_hvlip = WidgetCreator::getMenuItem(ID_ITEM_HVLIP);
  _menu->addItem(item_hvlip);

  Label* hvlip_type_lbl = WidgetCreator::getItemLabel(STR_BAT_TYPE_HVLIP, font_10x20);
  item_hvlip->setLabel(hvlip_type_lbl);

  // Залізо-фосфат
  MenuItem* lifepo_type_item = WidgetCreator::getMenuItem(ID_ITEM_LIFEPO);
  _menu->addItem(lifepo_type_item);

  Label* lifepo_type_lbl = WidgetCreator::getItemLabel(STR_BAT_TYPE_LIFEPO, font_10x20);
  lifepo_type_item->setLabel(lifepo_type_lbl);
}

void BattChargerContext::handleProfileBatTypeDialogState()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    uint16_t item_id = _menu->getCurrItemID();

    switch (item_id)
    {
      case ID_ITEM_LIP:
        _profile_edit.batt_type = BatType::BATT_TYPE_LIP;
        break;

      case ID_ITEM_HVLIP:
        _profile_edit.batt_type = BatType::BATT_TYPE_HVLIP;
        break;

      case ID_ITEM_LIFEPO:
        _profile_edit.batt_type = BatType::BATT_TYPE_LIFEPO;
        break;

      default:
        break;
    }

    showProfileEditTmpl();
    return;
  }

  if (_input.isReleased(BtnID::BTN_BACK))
  {
    showProfileEditTmpl();
    return;
  }

  if (_input.isHolded(BtnID::BTN_UP))
    _menu->focusUp();
  else if (_input.isHolded(BtnID::BTN_DOWN))
    _menu->focusDown();
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::updateVoltages()
{
  int16_t adc = _ads.readADC_SingleEnded(0);
  _bat_voltage = _ads.computeVolts(adc);
  // log_i("Bat voltage: %f V", _bat_voltage);

  adc = _ads.readADC_SingleEnded(2);
  _ads_out_voltage = _ads.computeVolts(adc);
  // log_i("Out voltage(ads): %f V", _ads_out_voltage);

  _ina_out_voltage = _ina.getBusVoltage();
  // log_i("Out voltage(ina): %f V", _ina_out_voltage);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::showChargeTmpl()  // TODO
{
  xTaskCreatePinnedToCore(wdt_task, "charg_wdt", 4 * 512, this, 10, &_wdt_task_handle, 0);
  if (!_wdt_task_handle)
  {
    log_e("%s", STR_ERR_START_TASK);
    showErrLabelTmpl(STR_ERR_START_TASK);
    return;
  }

  _state_handler = &BattChargerContext::handleChargeState;
}

void BattChargerContext::handleChargeState()  // TODO
{
  _was_wdt_fed = true;

  // if (_input.isPressed(BTN_BACK))
  // {
  //   vTaskDelete(_wdt_task_handle);
  //  _wdt_task_handle = nullptr;
  //   _readings_upd_ts = 0;
  //   // turnOffLoad();
  //   // turnOffFun();
  //   showMainTmpl();
  //   return;
  // }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::turnOnDcIn()
{
  digitalWrite(PIN_DC_IN_EN, HIGH);
}

void BattChargerContext::turnOffDcIn()
{
  digitalWrite(PIN_DC_IN_EN, LOW);
  turnOffDcOut();
}

void BattChargerContext::turnOnDcOut()
{
  digitalWrite(PIN_DC_OUT_EN, HIGH);
}

void BattChargerContext::turnOffDcOut()
{
  digitalWrite(PIN_DC_OUT_EN, LOW);
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::incVoltagePwm()
{
  if (_vout_pwm_duty == MAX_VOUT_PWM_DUTY)
    return;

  ++_vout_pwm_duty;
  ledcWrite(PIN_VOUT_PWM, _vout_pwm_duty);
}

void BattChargerContext::decVoltagePwm()
{
  if (_vout_pwm_duty == 0)
    return;

  --_vout_pwm_duty;
  ledcWrite(PIN_VOUT_PWM, _vout_pwm_duty);
}

// -----------------------------------------------------------------------------------------------------------------------

bool BattChargerContext::checkOutVoltageDiff()
{
  float volt_diff = std::abs(_ads_out_voltage - _ina_out_voltage);
  if (volt_diff > MAX_VOLT_DIFF)
  {
    log_e("Показники вихідної напруги INA219 та ADS відрізняються на %f В", volt_diff);
    showErrLabelTmpl(STR_ERR_VOLT_DIFF);
    return false;
  }

  return true;
}

void BattChargerContext::updateReadings()  // TODO
{
}

void BattChargerContext::updateCapacity(float current)
{
}

void BattChargerContext::updateChargeTime()
{
}

void BattChargerContext::adjustOutVoltagePWM()
{
}

// -----------------------------------------------------------------------------------------------------------------------

const char* BattChargerContext::getNameByBattType(BatType batt_type)
{
  switch (batt_type)
  {
    case BATT_TYPE_LIP:
      return STR_BAT_TYPE_HINT_LIP;

    case BATT_TYPE_HVLIP:
      return STR_BAT_TYPE_HINT_HVLIP;

    case BATT_TYPE_LIFEPO:
      return STR_BAT_TYPE_HINT_LIFEPO;

    default:
      return STR_NONE;
  }
}

// -----------------------------------------------------------------------------------------------------------------------

void BattChargerContext::wdt_task(void* arg)
{
  BattChargerContext* self = static_cast<BattChargerContext*>(arg);

  while (1)
  {
    delay(WDT_CHECK_DELAY);
    if (!self->_was_wdt_fed)
    {
      self->turnOffDcIn();
      log_e("Основна задача не відповідає. Можливо, датчики несправні. Заряджання зупинено");
      esp_restart();
    }

    self->_was_wdt_fed = false;
  }
}
