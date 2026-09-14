#include "ClientChessContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "manager/SettingsManager.h"
#include "scene/ClientChessScene.h"
#include "widget/text/TextBox.h"

namespace chess
{
  static const uint8_t ITEMS_PER_PAGE = 4;
  //----------------------------------------------------------------------------------------------------------

  ClientChessContext::ClientChessContext()
  {
    showAPScanTmpl();
  }

  ClientChessContext::~ClientChessContext()
  {
    _client.disconnect();

    _wifi.onScanComplete(nullptr, nullptr);
    _wifi.onConnectComplete(nullptr, nullptr);
  }

  //----------------------------------------------------------------------------------------------------------

  bool ClientChessContext::loop()
  {
    return true;
  }

  void ClientChessContext::update()
  {
    (this->*_state_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::showStateLabelTmpl(const String& msg_str)
  {
    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* state_lbl = new Label(ID_STATE_LBL);
    layout->addWidget(state_lbl);

    state_lbl->setText(msg_str);
    state_lbl->setWidth(UI_WIDTH);
    state_lbl->setAutoscroll(true);
    state_lbl->setBackColor(layout->getBackColor());
    state_lbl->setPos(0, getCenterY(state_lbl));
    state_lbl->setAlign(IWidget::ALIGN_CENTER);
    state_lbl->setGravity(IWidget::GRAVITY_CENTER);
  }
  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::startScanAP()
  {
    unsigned long ts = millis();
    while (_wifi.isBusy())
    {
      if (millis() - ts > 2000)
      {
        _wifi.disable();
        log_e("Модуль wifi не відповів вчасно");
        openContext(new ChessContext());
        _wifi.enable();
        return;
      }
      delay(10);
    }

    _wifi.onScanComplete(onScanCompleteHandler, this);
    _wifi.onConnectComplete(onApConnectHandler, this);

    _wifi.startScan();
  }

  void ClientChessContext::showAPScanTmpl()
  {
    _state_handler = &ClientChessContext::handleAPScanInput;
    showStateLabelTmpl(STR_AP_SCANNING);
    startScanAP();
  }

  void ClientChessContext::handleAPScanInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      openContext(new ChessContext());
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::showAPListTmpl()
  {
    std::vector<String> ssids = _wifi.getScanResult();
    if (ssids.empty())
    {
      showStateLabelTmpl(STR_AP_NOT_FOUND);
      return;
    }

    _state_handler = &ClientChessContext::handleAPListInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    FixedMenu* ssids_menu = new FixedMenu(ID_SSIDS_LIST);
    layout->addWidget(ssids_menu);
    ssids_menu->setBackColor(COLOR_MAIN_BACK);
    ssids_menu->setWidth(UI_WIDTH - DISPLAY_PADDING * 2);
    ssids_menu->setHeight(UI_HEIGHT);
    ssids_menu->setItemHeight(UI_HEIGHT / ITEMS_PER_PAGE - 2);
    ssids_menu->setPos(DISPLAY_PADDING, 0);
    ssids_menu->setLooped(true);

    for (size_t i = 0; i < ssids.size(); ++i)
    {
      MenuItem* item = WidgetCreator::getMenuItem(i + 1);
      ssids_menu->addItem(item);

      Label* item_lbl = WidgetCreator::getItemLabel(ssids[i]);
      item->setLabel(item_lbl);
    }
  }

  void ClientChessContext::handleAPListInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      openContext(new ChessContext());
    else if (_input.isReleased(BtnID::BTN_OK))
      connectToAP();
    else if (_input.isReleased(BtnID::BTN_UP))
      scrollSSIDsMenu(true);
    else if (_input.isReleased(BtnID::BTN_DOWN))
      scrollSSIDsMenu(false);
  }

  void ClientChessContext::connectToAP()
  {
    IWidget* raw_menu = getLayout()->getWidgetByID(ID_SSIDS_LIST);
    if (!raw_menu)
      return;

    FixedMenu* ssids_list = raw_menu->castTo<FixedMenu>();

    String selected_ssid = ssids_list->getCurrItemText();

    if (_wifi.hasKnownSSID(selected_ssid))
    {
      if (!_wifi.tryConnectToKnown(selected_ssid))
        return;

      showApConnectTmpl();
    }
    else
    {
      showPwdDialogTmpl(selected_ssid);
    }
  }

  void ClientChessContext::scrollSSIDsMenu(bool scroll_up)
  {
    IWidget* raw_menu = getLayout()->getWidgetByID(ID_SSIDS_LIST);
    if (!raw_menu)
      return;

    FixedMenu* ssids_list = raw_menu->castTo<FixedMenu>();

    if (scroll_up)
      ssids_list->focusUp();
    else
      ssids_list->focusDown();
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::showPwdDialogTmpl(const String& ssid)
  {
    _state_handler = &ClientChessContext::handlePwdDialogInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* title_lbl = new Label(ID_DIALOG_LBL);
    layout->addWidget(title_lbl);
    title_lbl->setText(ssid);
    title_lbl->setAlign(IWidget::ALIGN_CENTER);
    title_lbl->setGravity(IWidget::GRAVITY_CENTER);
    title_lbl->setWidth(UI_WIDTH);
    title_lbl->setBackColor(COLOR_MAIN_BACK);
    title_lbl->setTextColor(COLOR_ORANGE);
    title_lbl->setAutoscroll(true);

    TextBox* dialog_txt = new TextBox(ID_DIALOG_TEXT);
    layout->addWidget(dialog_txt);
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

  void ClientChessContext::handlePwdDialogInput()
  {
    IWidget* raw_kb = getLayout()->getWidgetByID(ID_DIALOG_KB);

    if (!raw_kb)
      return;

    Keyboard* keyboard = raw_kb->castTo<Keyboard>();
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
      Label* dialog_lbl = getLayout()->getWidgetByID(ID_DIALOG_LBL)->castTo<Label>();
      _wifi.saveSSID(dialog_lbl->getText(), tb_txt);
      _wifi.tryConnectToKnown(dialog_lbl->getText());
      showClientConnectTmpl();
    }
    else if (_input.isPressed(BtnID::BTN_BACK))
    {
      showAPScanTmpl();
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::showApConnectTmpl()
  {
    _state_handler = &ClientChessContext::handleClientConnectInput;
    showStateLabelTmpl(STR_AP_CONNECT);
  }

  void ClientChessContext::connectToServer()
  {
    _state_handler = &ClientChessContext::handleClientConnectInput;

    subscribeClientHandlers();

    String client_name = SettingsManager::get(STR_PREF_NICKNAME, STR_CHESS_GAME_DIR);
    if (client_name.isEmpty())
      client_name = STR_DEF_NAME;

    if (!_client.connect(client_name, STR_CHESS_GAME_ID))
    {
      showStateLabelTmpl(STR_SERVER_CONNECT_ERR);
      return;
    }

    showStateLabelTmpl(STR_SERVER_CONNECT);
  }

  void ClientChessContext::showClientConnectTmpl()
  {
    _state_handler = &ClientChessContext::handleClientConnectInput;
    showStateLabelTmpl(STR_SERVER_CONNECT);
  }

  void ClientChessContext::handleClientConnectInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      cancelClientConnect();
  }

  void ClientChessContext::subscribeClientHandlers()
  {
    _client.onConnect(onClientConnectHandler, this);
    _client.onDisconnect(onClientDisconnectHandler, this);
    _client.onGameStart(onGameStartHandler, this);
    _client.OnError(onClientErrorHandler, this);
  }

  void ClientChessContext::unsubscribeClientHandlers()
  {
    _client.onConnect(nullptr, nullptr);
    _client.onDisconnect(nullptr, nullptr);
    _client.onGameStart(nullptr, nullptr);
    _client.OnError(nullptr, nullptr);
  }

  void ClientChessContext::cancelClientConnect()
  {
    unsubscribeClientHandlers();
    _client.disconnect();
    showAPScanTmpl();
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::showLobbyTmpl()
  {
    _state_handler = &ClientChessContext::handleLobbyInput;
    showStateLabelTmpl(STR_WAITING_GAME);
  }

  void ClientChessContext::handleLobbyInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      cancelClientConnect();
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::startGame()
  {
    getLayout()->delWidgets();
    getLayout()->disable();

    unsubscribeClientHandlers();

    _scene = new ClientChessScene(_stored_objs, _client);
    _state_handler = &ClientChessContext::handleGame;
  }

  void ClientChessContext::handleGame()
  {
    if (!_scene->isReleased())
    {
      _scene->update();
    }
    else
    {
      delete _scene;

      if (_client.getStatus() == GameClient::STATUS_CONNECTED)
      {
        getLayout()->enable();
        subscribeClientHandlers();
        showLobbyTmpl();
      }
      else
      {
        openContext(new ChessContext());
      }
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::onClientConnectHandler(void* arg)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);
    self->post([self]()
               { self->showLobbyTmpl(); });
  }

  void ClientChessContext::onClientDisconnectHandler(void* arg)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);
    self->post([self]()
               { self->showStateLabelTmpl(STR_CLIENT_DISCONNECTED); });
  }

  void ClientChessContext::onClientErrorHandler(pixeler::GameClient::Error error, void* arg)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);

    String err_msg;

    switch (error)
    {
      case GameClient::Error::ERR_INCORRECT_SERVER:
        err_msg = STR_ERR_INCORRECT_SERVER;
        break;
      case GameClient::Error::ERR_INCORRECT_NAME:
        err_msg = STR_ERR_INCORRECT_NAME;
        break;
      case GameClient::Error::ERR_ACCESS_DENIED:
        err_msg = STR_ERR_ACCESS_DENIED;
        break;
      case GameClient::Error::ERR_SERVER_BUSY:
        err_msg = STR_ERR_SERVER_BUSY;
        break;

      default:
        err_msg = STR_ERR_UNKNOWN;
        break;
    }

    self->post([self, err_msg]()
               { self->showStateLabelTmpl(err_msg); });
  }

  void ClientChessContext::onGameStartHandler(void* arg)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);
    self->post([self]()
               { self->startGame(); });
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessContext::onScanCompleteHandler(void* arg)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);
    self->post([self]()
               { self->showAPListTmpl(); });
  }

  void ClientChessContext::onApConnectHandler(void* arg, const String& ssid, wl_status_t status)
  {
    ClientChessContext* self = static_cast<ClientChessContext*>(arg);

    if (status != WL_CONNECTED)
    {
      _wifi.forgetSSID(ssid);
      self->post([self, ssid]()
                 { self->showPwdDialogTmpl(ssid); });
    }
    else
    {
      self->post([self]()
                 { self->connectToServer(); });
    }
  }

  //----------------------------------------------------------------------------------------------------------
}  // namespace chess
