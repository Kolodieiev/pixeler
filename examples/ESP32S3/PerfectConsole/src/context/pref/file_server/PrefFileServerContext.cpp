#include "PrefFileServerContext.h"

#include "../../WidgetCreator.h"
#include "manager/SettingsManager.h"

bool PrefFileServerContext::loop()
{
  return true;
}

// cppcheck-suppress useInitializationList
PrefFileServerContext::PrefFileServerContext()
{
  _ssid_str = SettingsManager::get(STR_PREF_FS_AP_SSID);
  if (_ssid_str.isEmpty())
    _ssid_str = STR_DEF_SSID;

  _pwd_str = SettingsManager::get(STR_PREF_FS_AP_PWD);
  if (_pwd_str.isEmpty())
    _pwd_str = STR_DEF_PWD;

  showMainTmpl();
}

void PrefFileServerContext::showMainTmpl()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  //----------------------------------------------------

  const uint8_t LEFT_PADDING = 5;
  const uint8_t TOP_PADDING = 5;
  const uint8_t SPACE_BETWEEN = 40;

  _ssid_lbl = new Label(ID_LBL_SSID);
  layout->addWidget(_ssid_lbl);
  _ssid_lbl->setText(STR_AP_NAME);
  _ssid_lbl->setPos(LEFT_PADDING, TOP_PADDING);
  _ssid_lbl->setBackColor(COLOR_MAIN_BACK);
  _ssid_lbl->initWidthToFit();

  _ssid_txt = new TextBox(ID_TXT_SSID);
  layout->addWidget(_ssid_txt);
  _ssid_txt->setText(_ssid_str);
  _ssid_txt->setHPadding(5);
  _ssid_txt->setFocusBorderColor(COLOR_ORANGE);
  _ssid_txt->setChangingBorder(true);
  _ssid_txt->setTextSize(2);
  _ssid_txt->setHeight(_ssid_txt->getHeight() + 4);
  _ssid_txt->setWidth(UI_WIDTH - LEFT_PADDING * 2);
  _ssid_txt->setPos(LEFT_PADDING, _ssid_lbl->getYPos() + _ssid_lbl->getHeight() + 1);

  //----------------------------------------------------

  _pwd_lbl = _ssid_lbl->clone(ID_LBL_PWD);
  layout->addWidget(_pwd_lbl);
  _pwd_lbl->setText(STR_AP_PWD);
  _pwd_lbl->initWidthToFit();
  _pwd_lbl->setPos(LEFT_PADDING, _ssid_txt->getYPos() + _ssid_txt->getHeight() + SPACE_BETWEEN);

  _pwd_txt = _ssid_txt->clone(ID_TXT_PWD);
  layout->addWidget(_pwd_txt);
  _pwd_txt->setType(TextBox::TYPE_PASSWORD);
  _pwd_txt->setText(_pwd_str);
  _pwd_txt->setPos(LEFT_PADDING, _pwd_lbl->getYPos() + _pwd_lbl->getHeight() + 1);

  if (_is_ssid_edit)
  {
    _ssid_lbl->setTextColor(COLOR_ORANGE);
    _pwd_lbl->setTextColor(COLOR_WHITE);

    _ssid_txt->setFocus();
    _pwd_txt->removeFocus();
  }
  else
  {
    _ssid_lbl->setTextColor(COLOR_WHITE);
    _pwd_lbl->setTextColor(COLOR_ORANGE);

    _ssid_txt->removeFocus();
    _pwd_txt->setFocus();
  }

  _mode = MODE_MAIN;
}

void PrefFileServerContext::showDialogTmpl()
{
  IWidgetContainer* layout = getLayout();
  layout->disable();
  layout->delWidgets();
  layout->setBackColor(COLOR_BLACK);

    _dialog_txt = new TextBox(ID_TXT_DIALOG);
  layout->addWidget(_dialog_txt);
  _dialog_txt->setHPadding(5);
  _dialog_txt->setWidth(UI_WIDTH - 10);
  _dialog_txt->setHeight(40);
  _dialog_txt->setBackColor(COLOR_WHITE);
  _dialog_txt->setTextColor(COLOR_BLACK);
  _dialog_txt->setTextSize(2);
  _dialog_txt->setPos(5, 5);
  _dialog_txt->setCornerRadius(3);

  if (_is_ssid_edit)
    _dialog_txt->setText(_ssid_str);
  else
    _dialog_txt->setText(_pwd_str);

  _keyboard = WidgetCreator::getStandardEnKeyboard(ID_KEYBOARD);
  layout->addWidget(_keyboard);

  _mode = MODE_DIALOG;

  layout->enable();
}

void PrefFileServerContext::hideDialog()
{
  String dialog_res_str = _dialog_txt->getText();

  if (_is_ssid_edit)
    _ssid_str = dialog_res_str.isEmpty() ? STR_DEF_SSID : dialog_res_str;
  else
    _pwd_str = dialog_res_str.isEmpty() ? STR_DEF_PWD : dialog_res_str;

  showMainTmpl();
}

void PrefFileServerContext::saveSettings()
{
  SettingsManager::set(STR_PREF_FS_AP_SSID, _ssid_str.c_str());
  SettingsManager::set(STR_PREF_FS_AP_PWD, _pwd_str.c_str());
}

void PrefFileServerContext::up()
{
  if (_mode == MODE_MAIN)
    changeTbFocus();
  else
    _keyboard->focusUp();
}

void PrefFileServerContext::down()
{
  if (_mode == MODE_MAIN)
    changeTbFocus();
  else
    _keyboard->focusDown();
}

void PrefFileServerContext::left()
{
  if (_mode == MODE_DIALOG)
    _keyboard->focusLeft();
}

void PrefFileServerContext::right()
{
  if (_mode == MODE_DIALOG)
    _keyboard->focusRight();
}

void PrefFileServerContext::okPressed()
{
  if (_mode == MODE_MAIN)
  {
    saveSettings();
    release();
  }
  else if (_mode == MODE_DIALOG)
    hideDialog();
}

void PrefFileServerContext::ok()
{
  if (_mode == MODE_MAIN)
    showDialogTmpl();
  else if (_mode == MODE_DIALOG)
    keyboardClickHandler();
}

void PrefFileServerContext::keyboardClickHandler()
{
  _dialog_txt->addChars(_keyboard->getCurrBtnTxt().c_str());
}

void PrefFileServerContext::back()
{
  if (_mode == MODE_MAIN)
    release();
  else if (_mode == MODE_DIALOG)
    _dialog_txt->removeLastChar();
}

void PrefFileServerContext::backPressed()
{
  if (_mode == MODE_MAIN)
    release();
  else
    hideDialog();
}

void PrefFileServerContext::changeTbFocus()
{
  if (_is_ssid_edit)
  {
    _ssid_lbl->setTextColor(COLOR_WHITE);
    _pwd_lbl->setTextColor(COLOR_ORANGE);

    _ssid_txt->removeFocus();
    _pwd_txt->setFocus();
  }
  else
  {
    _ssid_lbl->setTextColor(COLOR_ORANGE);
    _pwd_lbl->setTextColor(COLOR_WHITE);

    _ssid_txt->setFocus();
    _pwd_txt->removeFocus();
  }

  _is_ssid_edit = !_is_ssid_edit;
}

void PrefFileServerContext::update()
{
  if (_input.isPressed(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, PRESS_LOCK);
    okPressed();
  }
  else if (_input.isPressed(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, PRESS_LOCK);
    backPressed();
  }
  else if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);
    up();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
    down();
  }
  else if (_input.isHolded(BtnID::BTN_RIGHT))
  {
    _input.lock(BtnID::BTN_RIGHT, HOLD_LOCK);
    right();
  }
  else if (_input.isHolded(BtnID::BTN_LEFT))
  {
    _input.lock(BtnID::BTN_LEFT, HOLD_LOCK);
    left();
  }
  else if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    ok();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    back();
  }
}
