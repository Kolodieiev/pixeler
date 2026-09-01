#include "ChessPrefContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "manager/SettingsManager.h"
#include "widget/text/TextBox.h"

namespace chess
{
  static const uint8_t MENU_ITEM_NUM{5};

  ChessPrefContext::ChessPrefContext()
  {
    setCpuFrequency(FREQ_MIN);

    showMainTmpl();
  }

  ChessPrefContext::~ChessPrefContext()
  {
  }

  bool ChessPrefContext::loop()
  {
    return true;
  }

  void ChessPrefContext::update()
  {
    (this->*_state_input_handler)();
  }

  void ChessPrefContext::showMainTmpl()
  {
    _state_input_handler = &ChessPrefContext::procMainMenu;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    FixedMenu* menu = new FixedMenu(ID_MAIN_MENU);
    layout->addWidget(menu);
    menu->setBackColor(COLOR_MAIN_BACK);
    menu->setWidth(UI_WIDTH);
    menu->setHeight(UI_HEIGHT);
    menu->setItemHeight(UI_HEIGHT / MENU_ITEM_NUM - 2);
    menu->setLooped(true);

    // nick
    MenuItem* nick_item = WidgetCreator::getMenuItem(ID_ITEM_NICK);
    menu->addItem(nick_item);

    Label* nick_lbl = WidgetCreator::getItemLabel(STR_NICKNAME, font_10x20);
    nick_item->setLabel(nick_lbl);

    // serv name
    MenuItem* serv_name_item = WidgetCreator::getMenuItem(ID_ITEM_SERV_NAME);
    menu->addItem(serv_name_item);

    Label* serv_name_lbl = WidgetCreator::getItemLabel(STR_SERV_NAME, font_10x20);
    serv_name_item->setLabel(serv_name_lbl);

    // serv pwd
    MenuItem* serv_pwd_item = WidgetCreator::getMenuItem(ID_ITEM_SERV_PWD);
    menu->addItem(serv_pwd_item);

    Label* serv_pwd_lbl = WidgetCreator::getItemLabel(STR_SERV_PWD, font_10x20);
    serv_pwd_item->setLabel(serv_pwd_lbl);
  }

  void ChessPrefContext::procMainMenu()
  {
    FixedMenu* menu = getLayout()->getWidgetByID(ID_MAIN_MENU)->castTo<FixedMenu>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      menu->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      menu->focusDown();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      uint16_t id = menu->getCurrItemID();

      switch (id)
      {
        case ID_ITEM_NICK:
          showDialogNicknameTmpl();
          break;

        case ID_ITEM_SERV_NAME:
          showDialogServNameTmpl();
          break;

        case ID_ITEM_SERV_PWD:
          showDialogServPwdTmpl();
          break;
      }
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      openContext(new ChessContext());
    }
  }

  void ChessPrefContext::showDialogNicknameTmpl()
  {
    String _client_nick = SettingsManager::get(STR_PREF_NICKNAME, STR_CHESS_GAME_DIR);
    _dialog_id = DIALOG_ID_NICK;
    addDialog(STR_NICKNAME, _client_nick);
  }

  void ChessPrefContext::showDialogServNameTmpl()
  {
    String _serv_ssid = SettingsManager::get(STR_PREF_SERVER_SSID, STR_CHESS_GAME_DIR);
    _dialog_id = DIALOG_ID_SERV_NAME;
    addDialog(STR_SERV_NAME, _serv_ssid);
  }

  void ChessPrefContext::showDialogServPwdTmpl()
  {
    String _serv_pwd = SettingsManager::get(STR_PREF_SERVER_PWD, STR_CHESS_GAME_DIR);
    _dialog_id = DIALOG_ID_SERV_PWD;
    addDialog(STR_SERV_PWD, _serv_pwd);
  }

  void ChessPrefContext::addDialog(const String& title_txt, const String& start_txt)
  {
    _state_input_handler = &ChessPrefContext::handleDialogInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* title_lbl = new Label(ID_DIALOG_LBL);
    layout->addWidget(title_lbl);
    title_lbl->setText(title_txt);
    title_lbl->setAlign(IWidget::ALIGN_CENTER);
    title_lbl->setGravity(IWidget::GRAVITY_CENTER);
    title_lbl->setWidth(UI_WIDTH);
    title_lbl->setBackColor(COLOR_MAIN_BACK);
    title_lbl->setTextColor(COLOR_ORANGE);
    title_lbl->setAutoscroll(true);

    TextBox* dialog_txt = new TextBox(ID_DIALOG_TEXT);
    layout->addWidget(dialog_txt);
    dialog_txt->setText(start_txt);
    dialog_txt->setHPadding(5);
    dialog_txt->setWidth(UI_WIDTH - 10);
    dialog_txt->setHeight(40);
    dialog_txt->setBackColor(COLOR_WHITE);
    dialog_txt->setTextColor(COLOR_BLACK);
    dialog_txt->setTextSize(2);
    dialog_txt->setPos(5, title_lbl->getYPos() + title_lbl->getHeight() + 5);
    dialog_txt->setCornerRadius(3);

    Keyboard* keyboard = WidgetCreator::getStandardEnKeyboard(ID_DIALOG_KB);
    layout->addWidget(keyboard);
    keyboard->setPos(0, dialog_txt->getYPos() + dialog_txt->getHeight() + 5);
  }

  void ChessPrefContext::handleDialogInput()
  {
    Keyboard* keyboard = getLayout()->getWidgetByID(ID_DIALOG_KB)->castTo<Keyboard>();
    TextBox* dialog_txt = getLayout()->getWidgetByID(ID_DIALOG_TEXT)->castTo<TextBox>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      keyboard->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      keyboard->focusDown();
    }
    else if (_input.isHolded(BtnID::BTN_LEFT))
    {
      keyboard->focusLeft();
    }
    else if (_input.isHolded(BtnID::BTN_RIGHT))
    {
      keyboard->focusRight();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      dialog_txt->addChars(keyboard->getCurrBtnTxt().c_str());
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      dialog_txt->removeLastChar();
    }
    else if (_input.isPressed(BtnID::BTN_OK))
    {

      String tb_txt = dialog_txt->getText();
      saveDialogResult(tb_txt);
    }
    else if (_input.isPressed(BtnID::BTN_BACK))
    {
      showMainTmpl();  // Повертаємось до вікна налаштувань
    }
  }

  void ChessPrefContext::saveDialogResult(String& result_str)
  {
    const char* PREF_FIELD_NAME = nullptr;
    switch (_dialog_id)
    {
      case DIALOG_ID_NICK:
        PREF_FIELD_NAME = STR_PREF_NICKNAME;
        break;

      case DIALOG_ID_SERV_NAME:
        PREF_FIELD_NAME = STR_PREF_SERVER_SSID;
        break;

      case DIALOG_ID_SERV_PWD:
        PREF_FIELD_NAME = STR_PREF_SERVER_PWD;
        break;
    }

    showMainTmpl();

    if (PREF_FIELD_NAME && SettingsManager::set(PREF_FIELD_NAME, result_str, STR_CHESS_GAME_DIR))
      showToast(STR_SUCCESS);
    else
      showToast(STR_FAIL);
  }
}  // namespace chess
