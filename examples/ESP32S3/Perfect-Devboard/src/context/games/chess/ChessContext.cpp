#include "ChessContext.h"

#include "../../WidgetCreator.h"
#include "manager/SettingsManager.h"
#include "widget/text/TextBox.h"
#include "scene/ChessScene.h"

static const char STR_CHESS_GAME_DIR[] = "chess";

static const char STR_SELECT_ROLE_TITLE[] = "Оберіть роль";
static const char STR_WAITING_CLIENT[] = "Очікуйте приєднання клієнтів";
static const char STR_SERVER_SCANNING[] = "Зачекайте, відбувається сканування";
static const char STR_SERVER_SCANNING_DONE[] = "Сканування завершено";
static const char STR_SELECT_SERVER[] = "Оберіть сервер";
static const char STR_CLIENT[] = "Клієнт";
static const char STR_SERVER[] = "Сервер";
static const char STR_ENTER_PWD[] = "Введіть пароль до: ";
static const char STR_CONNECTING_TO[] = "Очікуємо підключення до: ";

static const char STR_SERVER_UNAVAILABLE[] = "Сервер не відповідає";
static const char STR_CONNECTING_ERROR[] = "Помилка підключення";
static const char STR_CONNECTING[] = "Підключення до сервера";

static const char STR_WANTS_TO_JOIN[] = " хоче приєднатися";

static const char STR_WAITING_GAME[] = "Очікуйте запуск гри";
static const char STR_DISCONNECTED[] = "Від'єднано від сервера";
//
static const char STR_CONT_DISC_CLIENT[] = "Відключити клієнта";
static const char STR_CONT_OPEN_LOBBY[] = "Відкрити лоббі";
static const char STR_CONT_CLOSE_LOBBY[] = "Закрити лоббі";
static const char STR_CONT_GAME_START[] = "Розпочати гру";
static const char STR_CONT_GAME_EXIT[] = "Завершити гру";

static const uint8_t ITEM_NUM{5};

namespace chess
{
  //----------------------------------------------------------------------------------------------------------

  ChessContext::ChessContext()
  {
    setCpuFrequencyMhz(BALANCED_CPU_FREQ_MHZ);
    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);
    showMainTmpl();
    loadPrefs();
  }

  ChessContext::~ChessContext()
  {
    setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
  }

  //----------------------------------------------------------------------------------------------------------

  bool ChessContext::loop()
  {
    return true;
  }

  void ChessContext::update()
  {
    switch (_mode)
    {
      case MODE_GAME:
        updateGame();
        break;

      case MODE_MAIN:
        procKbMain();
        break;

      case MODE_WIFI_SCAN:
        procKbWifiScan();
        break;

      case MODE_WIFI_LIST:
        procKbWifiList();
        break;

      case MODE_CONN_TO_AP:
        procKbConnToAp();
        break;

      case MODE_CLIENT_LOBBY:
        procKbClientLobby();
        break;

      case MODE_SERVER_LOBBY:
        procKbServerLobby();
        break;

      case MODE_LOBBY_CTXT_MENU:
        procKbCtxtLobby();
        break;

      case MODE_CLIENT_CONFIRM:
        procKbClientConfirm();
        break;

      case MODE_PREF_MAIN:
        procKbPrefMain();
        break;

      case MODE_CONN_DIALOG:
      case MODE_PREF_NICK:
      case MODE_PREF_SERV_NAME:
      case MODE_PREF_SERV_PWD:
        procKbPrefDialog();
        break;
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessContext::loadPrefs()
  {
    _client_nick = SettingsManager::get(STR_PREF_NICKNAME, STR_CHESS_GAME_DIR);
    _serv_ssid = SettingsManager::get(STR_PREF_SERVER_SSID, STR_CHESS_GAME_DIR);
    _serv_pwd = SettingsManager::get(STR_PREF_SERVER_PWD, STR_CHESS_GAME_DIR);
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessContext::startGame(GameMode mode)
  {
    _mode = MODE_GAME;
    getLayout()->delWidgets();
    switch (mode)
    {  // TODO
      case GAME_MODE_SERVER:
        break;

      case GAME_MODE_CLIENT:
        // _is_client = true;
        break;

      case GAME_MODE_TWO_PL:
        _scene = new ChessScene(_stored_objs, 2);
        break;

      default:  // GAME_MODE_ONE_PL
        _scene = new ChessScene(_stored_objs, 1);
        break;
    }
  }

  void ChessContext::updateGame()
  {
    _scene->update();

    if (!_scene->isFinished() && !_scene->isReleased())
    {
      _scene->update();
    }
    else
    {
      // if (_is_client) // TODO
      //   _client.disconnect();
      // else if (_is_server)
      //   _server.stop();

      delete _scene;

      showMainTmpl();
    }
  }

  //----------------------------------------------------------------------------------------------------------
  //----------------------------------------------------------------------------------------------------------

  void ChessContext::showMainTmpl()
  {
    _mode = MODE_MAIN;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    FixedMenu* menu = new FixedMenu(ID_MAIN_MENU);
    layout->addWidget(menu);
    menu->setBackColor(COLOR_MAIN_BACK);
    menu->setWidth(UI_WIDTH);
    menu->setHeight(UI_HEIGHT);
    menu->setItemHeight(UI_HEIGHT / ITEM_NUM - 2);
    menu->setLoopState(true);

    // Один гравець
    MenuItem* solo_item = WidgetCreator::getMenuItem(ID_ITEM_ONE_PLAYER);
    menu->addItem(solo_item);

    Label* solo_lbl = WidgetCreator::getItemLabel(STR_MODE_ONE_PL, font_10x20);
    solo_item->setLbl(solo_lbl);

    // Два гравці
    MenuItem* multi_item = WidgetCreator::getMenuItem(ID_ITEM_TWO_PLAYERS);
    menu->addItem(multi_item);

    Label* multi_lbl = WidgetCreator::getItemLabel(STR_MODE_TWO_PL, font_10x20);
    multi_item->setLbl(multi_lbl);

    // Клієнт
    MenuItem* client_item = WidgetCreator::getMenuItem(ID_ITEM_CLIENT);
    menu->addItem(client_item);

    Label* client_lbl = WidgetCreator::getItemLabel(STR_MODE_CLIENT, font_10x20);
    client_item->setLbl(client_lbl);

    // Сервер
    MenuItem* server_item = WidgetCreator::getMenuItem(ID_ITEM_SERVER);
    menu->addItem(server_item);

    Label* server_lbl = WidgetCreator::getItemLabel(STR_MODE_SERVER, font_10x20);
    server_item->setLbl(server_lbl);

    // Налаштування
    MenuItem* prefs_item = WidgetCreator::getMenuItem(ID_ITEM_PREFS);
    menu->addItem(prefs_item);

    Label* prefs_lbl = WidgetCreator::getItemLabel(STR_PREFERENCES, font_10x20);
    prefs_item->setLbl(prefs_lbl);
  }

  void ChessContext::showPrefsTmpl()
  {
    _mode = MODE_PREF_MAIN;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    FixedMenu* menu = new FixedMenu(ID_MAIN_MENU);
    layout->addWidget(menu);
    menu->setBackColor(COLOR_MAIN_BACK);
    menu->setWidth(UI_WIDTH);
    menu->setHeight(UI_HEIGHT);
    menu->setItemHeight(UI_HEIGHT / ITEM_NUM - 2);
    menu->setLoopState(true);

    // nick
    MenuItem* nick_item = WidgetCreator::getMenuItem(ID_ITEM_NICK);
    menu->addItem(nick_item);

    Label* nick_lbl = WidgetCreator::getItemLabel(STR_NICKNAME, font_10x20);
    nick_item->setLbl(nick_lbl);

    // serv name
    MenuItem* serv_name_item = WidgetCreator::getMenuItem(ID_ITEM_SERV_NAME);
    menu->addItem(serv_name_item);

    Label* serv_name_lbl = WidgetCreator::getItemLabel(STR_SERV_NAME, font_10x20);
    serv_name_item->setLbl(serv_name_lbl);

    // serv pwd
    MenuItem* serv_pwd_item = WidgetCreator::getMenuItem(ID_ITEM_SERV_PWD);
    menu->addItem(serv_pwd_item);

    Label* serv_pwd_lbl = WidgetCreator::getItemLabel(STR_SERV_PWD, font_10x20);
    serv_pwd_item->setLbl(serv_pwd_lbl);
  }

  void ChessContext::showPrefsNickTmpl()
  {
    _mode = MODE_PREF_NICK;
    addDialog(STR_NICKNAME, _client_nick);
  }

  void ChessContext::showPrefsServNameTmpl()
  {
    _mode = MODE_PREF_SERV_NAME;
    addDialog(STR_SERV_NAME, _serv_ssid);
  }

  void ChessContext::showPrefsServPwdTmpl()
  {
    _mode = MODE_PREF_SERV_PWD;
    addDialog(STR_SERV_PWD, _serv_pwd);
  }

  void ChessContext::addDialog(const String& title_txt, const String& start_txt)
  {
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

  void ChessContext::showConnDialogTmpl()
  {
    _mode = MODE_CONN_DIALOG;
    addDialog(STR_NICKNAME, _client_nick);  // TODO тут відображати імя SSID
  }

  void ChessContext::showNewClientConnTmpl()  // TODO
  {
  }

  void ChessContext::showServerCtxTmpl()
  {
  }

  void ChessContext::showServerLobbyTmpl()
  {
  }

  void ChessContext::showWifiScanTmpl()
  {
  }

  void ChessContext::showWifiListTmpl()
  {
  }

  void ChessContext::showConnToApTmpl()
  {
  }

  void ChessContext::showClientLobbyTmpl()
  {
  }

  //----------------------------------------------------------------------------------------------------------
  //----------------------------------------------------------------------------------------------------------

  void ChessContext::procKbMain()
  {
    FixedMenu* menu = getLayout()->getWidgetByID(ID_MAIN_MENU)->castTo<FixedMenu>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      _input.lock(BtnID::BTN_UP, HOLD_LOCK);
      menu->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
      menu->focusDown();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      _input.lock(BtnID::BTN_OK, CLICK_LOCK);
      uint16_t id = menu->getCurrItemID();

      switch (id)
      {
        // case ID_ITEM_ONE_PLAYER: // TODO
        //   startGame(GAME_MODE_ONE_PL);
        //   break;

        case ID_ITEM_TWO_PLAYERS:
          startGame(GAME_MODE_TWO_PL);
          break;

        case ID_ITEM_CLIENT:
          showClientLobbyTmpl();
          break;

        case ID_ITEM_SERVER:
          showServerLobbyTmpl();
          break;

        case ID_ITEM_PREFS:
          showPrefsTmpl();
          break;
      }
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      openContextByID(ID_CONTEXT_GAMES);
    }
  }

  void ChessContext::procKbPrefMain()
  {
    FixedMenu* menu = getLayout()->getWidgetByID(ID_MAIN_MENU)->castTo<FixedMenu>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      _input.lock(BtnID::BTN_UP, HOLD_LOCK);
      menu->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
      menu->focusDown();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      _input.lock(BtnID::BTN_OK, CLICK_LOCK);
      uint16_t id = menu->getCurrItemID();

      switch (id)
      {
        case ID_ITEM_NICK:
          showPrefsNickTmpl();
          break;

        case ID_ITEM_SERV_NAME:
          showPrefsServNameTmpl();
          break;

        case ID_ITEM_SERV_PWD:
          showPrefsServPwdTmpl();
          break;
      }
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, HOLD_LOCK);
      showMainTmpl();
    }
  }

  void ChessContext::procKbPrefDialog()
  {
    Keyboard* keyboard = getLayout()->getWidgetByID(ID_DIALOG_KB)->castTo<Keyboard>();
    TextBox* dialog_txt = getLayout()->getWidgetByID(ID_DIALOG_TEXT)->castTo<TextBox>();

    if (_input.isHolded(BtnID::BTN_UP))
    {
      _input.lock(BtnID::BTN_UP, CLICK_LOCK);
      keyboard->focusUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      _input.lock(BtnID::BTN_DOWN, CLICK_LOCK);
      keyboard->focusDown();
    }
    else if (_input.isHolded(BtnID::BTN_LEFT))
    {
      _input.lock(BtnID::BTN_LEFT, CLICK_LOCK);
      keyboard->focusLeft();
    }
    else if (_input.isHolded(BtnID::BTN_RIGHT))
    {
      _input.lock(BtnID::BTN_RIGHT, CLICK_LOCK);
      keyboard->focusRight();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      _input.lock(BtnID::BTN_OK, CLICK_LOCK);
      dialog_txt->addChars(keyboard->getCurrBtnTxt().c_str());
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      dialog_txt->removeLastChar();
    }
    else if (_input.isPressed(BtnID::BTN_OK))
    {
      _input.lock(BtnID::BTN_OK, PRESS_LOCK);

      bool save_pref_result{false};

      switch (_mode)
      {
        case MODE_CONN_DIALOG:  // Якщо в режимі підключення до сервера
          _serv_pwd = dialog_txt->getText();
          SettingsManager::set(STR_PREF_SERVER_PWD, _serv_pwd.c_str(), STR_CHESS_GAME_DIR);
          showConnToApTmpl();
          return;

        case MODE_PREF_NICK:
          _client_nick = dialog_txt->getText();
          save_pref_result = SettingsManager::set(STR_PREF_NICKNAME, _client_nick.c_str(), STR_CHESS_GAME_DIR);
          break;

        case MODE_PREF_SERV_NAME:
          _serv_ssid = dialog_txt->getText();
          save_pref_result = SettingsManager::set(STR_PREF_SERVER_SSID, _serv_ssid.c_str(), STR_CHESS_GAME_DIR);
          break;

        case MODE_PREF_SERV_PWD:
          _serv_pwd = dialog_txt->getText();
          save_pref_result = SettingsManager::set(STR_PREF_SERVER_PWD, _serv_pwd.c_str(), STR_CHESS_GAME_DIR);
          break;
      }

      showPrefsTmpl();

      if (save_pref_result)
        showToast(STR_SUCCESS);
      else
        showToast(STR_FAIL);
    }
    else if (_input.isPressed(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, PRESS_LOCK);

      if (_mode == MODE_CONN_DIALOG)  // Якщо в режимі підключення до сервера
        showWifiScanTmpl();           // Пароль не введено. Повторно запускаємо сканування
      else
        showPrefsTmpl();
    }
  }

  void ChessContext::procKbWifiScan()  // TODO
  {
  }

  void ChessContext::procKbWifiList()
  {
  }

  void ChessContext::procKbClientLobby()
  {
  }

  void ChessContext::procKbConnToAp()
  {
  }

  void ChessContext::procKbServerLobby()
  {
  }

  void ChessContext::procKbCtxtLobby()
  {
  }

  void ChessContext::procKbClientConfirm()
  {
  }

  //----------------------------------------------------------------------------------------------------------
  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
