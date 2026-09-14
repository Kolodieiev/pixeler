#include "ServerChessContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "manager/SettingsManager.h"
#include "manager/WiFiManager.h"
#include "scene/ServerChessScene.h"

namespace chess
{
  static const uint8_t PADDING_BOTT = 40;
  static const uint8_t MAX_CLIENT_NUM = 3;

  ServerChessContext::ServerChessContext()
  {
    _wifi_was_enabled = _wifi.isEnabled();

    showLobbyTmpl();
    setupServer();
  }

  ServerChessContext::~ServerChessContext()
  {
    if (!_wifi_was_enabled)
      _wifi.disable();
  }

  void ServerChessContext::setupServer()
  {
    String server_name = SettingsManager::get(STR_PREF_SERVER_SSID, STR_CHESS_GAME_DIR);
    if (server_name.isEmpty())
      server_name = STR_DEF_SSID;

    String server_pwd = SettingsManager::get(STR_PREF_SERVER_PWD, STR_CHESS_GAME_DIR);
    if (server_pwd.isEmpty())
      server_pwd = STR_DEF_PWD;

    subscribeServerHandlers();

    _server.begin(STR_CHESS_GAME_ID, server_name, server_pwd, MAX_CLIENT_NUM);
    _server.open();
  }
  //----------------------------------------------------------------------------------------------------------

  bool ServerChessContext::loop()
  {
    return true;
  }

  void ServerChessContext::update()
  {
    (this->*_state_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessContext::showLobbyTmpl()
  {
    _state_handler = &ServerChessContext::handleLobbyInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    const std::unordered_map<uint32_t, pixeler::ClientSession>* clients = _server.getClients();

    if (clients->empty())
    {
      Label* empty_lobby_msg = new Label(ID_EMPTY_MSG);
      layout->addWidget(empty_lobby_msg);

      empty_lobby_msg->setText(STR_MISSING_CLIENTS);
      empty_lobby_msg->setWidth(UI_WIDTH);
      empty_lobby_msg->setAutoscroll(true);
      empty_lobby_msg->setBackColor(layout->getBackColor());
      empty_lobby_msg->setPos(0, getCenterY(empty_lobby_msg));
      empty_lobby_msg->setAlign(IWidget::ALIGN_CENTER);
      empty_lobby_msg->setGravity(IWidget::GRAVITY_CENTER);

      return;
    }

    FixedMenu* clients_menu = new FixedMenu(ID_CLIENT_LIST);
    layout->addWidget(clients_menu);
    clients_menu->setBackColor(COLOR_MAIN_BACK);
    clients_menu->setWidth(UI_WIDTH - DISPLAY_PADDING * 2);
    clients_menu->setHeight(UI_HEIGHT);
    clients_menu->setItemHeight(UI_HEIGHT / 4 - 2);
    clients_menu->setPos(DISPLAY_PADDING, 0);
    clients_menu->setLooped(true);

    uint16_t item_id_counter = 1;

    for (auto it_b = clients->begin(), it_e = clients->end(); it_b != it_e; ++it_b)
    {
      MenuItem* item = WidgetCreator::getMenuItem(item_id_counter);
      clients_menu->addItem(item);

      Label* item_lbl = WidgetCreator::getItemLabel(it_b->second.getName());
      item->setLabel(item_lbl);

      ++item_id_counter;
    }
  }

  void ServerChessContext::handleLobbyInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      openContext(new ChessContext());
    else if (_input.isPressed(BtnID::BTN_OK))
      showLobbyContextMenuTmpl();
    else if (_input.isReleased(BtnID::BTN_UP))
      scrollClientsMenu(true);
    else if (_input.isReleased(BtnID::BTN_DOWN))
      scrollClientsMenu(false);
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessContext::showLobbyContextMenuTmpl()
  {
    _state_handler = &ServerChessContext::handleContextMenuInput;

    FixedMenu* context_menu = WidgetCreator::getContextMenu(ID_CONTEXT_MENU);
    getLayout()->addWidget(context_menu);

    // toggle lobby
    MenuItem* switch_item = WidgetCreator::getMenuItem(ID_ITEM_TOGGLE_LOBBY);
    context_menu->addItem(switch_item);
    Label* switch_lbl = WidgetCreator::getItemLabel(emptyString, font_10x20);
    switch_item->setLabel(switch_lbl);

    if (_server.isOpen())
      switch_lbl->setText(STR_CLOSE_LOBBY);
    else
      switch_lbl->setText(STR_OPEN_LOBBY);

    IWidget* raw_menu = getLayout()->getWidgetByID(ID_CLIENT_LIST);
    if (raw_menu)
    {
      FixedMenu* clients_menu = raw_menu->castTo<FixedMenu>();
      if (clients_menu->getSize() > 0)
      {
        clients_menu->disable();

        // start game
        MenuItem* start_game_item = WidgetCreator::getMenuItem(ID_ITEM_START_GAME);
        context_menu->addItem(start_game_item);
        Label* start_game_lbl = WidgetCreator::getItemLabel(STR_START_GAME, font_10x20);
        start_game_item->setLabel(start_game_lbl);

        // kick client
        MenuItem* kick_client_item = WidgetCreator::getMenuItem(ID_ITEM_KICK_CLIENT);
        context_menu->addItem(kick_client_item);
        Label* kick_client_lbl = WidgetCreator::getItemLabel(STR_KICK_CLIENT, font_10x20);
        kick_client_item->setLabel(kick_client_lbl);
      }
    }

    context_menu->setHeight(context_menu->getSize() * context_menu->getItemHeight() + 4);
    context_menu->setPos(UI_WIDTH - context_menu->getWidth() - 1,
                         UI_HEIGHT - PADDING_BOTT - context_menu->getHeight() - 2);
  }

  void ServerChessContext::hideLobbyContextMenu()
  {
    _state_handler = &ServerChessContext::handleLobbyInput;

    getLayout()->delWidgetByID(ID_CONTEXT_MENU);

    IWidget* raw_menu = getLayout()->getWidgetByID(ID_CLIENT_LIST);
    if (raw_menu)
    {
      FixedMenu* clients_menu = raw_menu->castTo<FixedMenu>();
      clients_menu->enable();
    }
  }

  void ServerChessContext::handleContextMenuInput()
  {
    if (_input.isReleased(BtnID::BTN_BACK))
    {
      hideLobbyContextMenu();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      FixedMenu* context_menu = getLayout()->getWidgetByID(ID_CONTEXT_MENU)->castTo<FixedMenu>();
      uint16_t item_id = context_menu->getCurrItemID();

      switch (item_id)
      {
        case ID_ITEM_TOGGLE_LOBBY:
          _server.toggle();
          hideLobbyContextMenu();
          break;

        case ID_ITEM_START_GAME:
        {
          FixedMenu* client_list = getLayout()->getWidgetByID(ID_CLIENT_LIST)->castTo<FixedMenu>();
          startGame(client_list->getCurrItemText());
        }
        break;

        case ID_ITEM_KICK_CLIENT:
        {
          FixedMenu* client_list = getLayout()->getWidgetByID(ID_CLIENT_LIST)->castTo<FixedMenu>();
          _server.removeSession(client_list->getCurrItemText()); 
          // Оновлення view буде викликано через disconnectHandler
        }
        break;

        default:
          break;
      }
    }
    else if (_input.isReleased(BtnID::BTN_UP))
    {
      FixedMenu* context_menu = getLayout()->getWidgetByID(ID_CONTEXT_MENU)->castTo<FixedMenu>();
      context_menu->focusUp();
    }
    else if (_input.isReleased(BtnID::BTN_DOWN))
    {
      FixedMenu* context_menu = getLayout()->getWidgetByID(ID_CONTEXT_MENU)->castTo<FixedMenu>();
      context_menu->focusDown();
    }
  }

  void ServerChessContext::scrollClientsMenu(bool scroll_up)
  {
    IWidget* raw_menu = getLayout()->getWidgetByID(ID_CLIENT_LIST);
    if (raw_menu)
    {
      FixedMenu* clients_menu = raw_menu->castTo<FixedMenu>();

      if (scroll_up)
        clients_menu->focusUp();
      else
        clients_menu->focusDown();
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessContext::showClientAcceptTmpl(String client_name)
  {
    _state_handler = &ServerChessContext::handleClientAcceptInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* accept_title = new Label(ID_LBL_ACCEPT_TITLE);
    layout->addWidget(accept_title);
    accept_title->setText(STR_WANTS_TO_JOIN);
    accept_title->setBackColor(COLOR_MAIN_BACK);
    accept_title->setGravity(IWidget::GRAVITY_CENTER);
    accept_title->setWidth(UI_WIDTH);
    accept_title->setHeight(20);
    accept_title->setAutoscroll(true);

    Label* name_lbl = accept_title->clone(ID_LBL_CLIENT_NAME);
    layout->addWidget(name_lbl);
    name_lbl->setText(client_name);
    name_lbl->setPos(0, accept_title->getBottomYPos() + 5);
    name_lbl->setFont(font_inr24);
    name_lbl->setTextColor(COLOR_GREEN);

    Label* accept_way = accept_title->clone(ID_LBL_ACCEPT_WAY);
    layout->addWidget(accept_way);
    accept_way->setText(STR_ACCEPT_WAY);
    accept_way->setPos(0, name_lbl->getBottomYPos() + 5);

    Label* reject_way = accept_title->clone(ID_LBL_REJECT_WAY);
    layout->addWidget(reject_way);
    reject_way->setText(STR_REJECT_WAY);
    reject_way->setPos(0, accept_way->getBottomYPos() + 5);
  }

  void ServerChessContext::handleClientAcceptInput()
  {
    if (_input.isReleased(BtnID::BTN_BACK))
      handleClientAcceptResult(false);
    else if (_input.isReleased(BtnID::BTN_OK))
      handleClientAcceptResult(true);
  }

  void ServerChessContext::handleClientAcceptResult(bool is_accepted)
  {
    Label* client_name_lbl = getLayout()->getWidgetByID(ID_LBL_CLIENT_NAME)->castTo<Label>();
    _server.resolveJoin(client_name_lbl->getText(), is_accepted);
    showLobbyTmpl();
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessContext::onAcceptHandler(const String client_name, void* arg)
  {
    ServerChessContext* self = static_cast<ServerChessContext*>(arg);
    self->post([self, client_name]()
               { self->showClientAcceptTmpl(client_name); }, 500);
  }

  void ServerChessContext::onDisconnectHandler(const IPAddress& client_ip, void* arg)
  {
    ServerChessContext* self = static_cast<ServerChessContext*>(arg);
    self->post([self]()
               { self->showLobbyTmpl(); }, 500);
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessContext::subscribeServerHandlers()
  {
    _server.onAccept(onAcceptHandler, this);
    _server.onDisconnect(onDisconnectHandler, this);
  }

  void ServerChessContext::unsubscribeServerHandlers()
  {
    _server.onAccept(nullptr, nullptr);
    _server.onDisconnect(nullptr, nullptr);
  }

  void ServerChessContext::startGame(const String& main_client_name)
  {
    _server.close();
    unsubscribeServerHandlers();
    _state_handler = &ServerChessContext::handleGame;
    IPAddress main_ip = _server.getClientIP(main_client_name);
    getLayout()->delWidgets();
    getLayout()->disable();
    _scene = new ServerChessScene(_stored_objs, _server, main_ip);
  }

  void ServerChessContext::handleGame()
  {
    if (!_scene->isReleased())
    {
      _scene->update();
    }
    else
    {
      delete _scene;
      _scene = nullptr;
      subscribeServerHandlers();
      getLayout()->enable();
      showLobbyTmpl();
      _server.open();
    }
  }

  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
