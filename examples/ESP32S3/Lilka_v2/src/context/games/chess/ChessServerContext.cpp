#include "ChessServerContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "manager/SettingsManager.h"

namespace chess
{
  const uint16_t PADDING_BOTT = 40;

  ChessServerContext::ChessServerContext()
  {
    showLobbyTmpl();

    _server.onConfirmation(onConfirmationHandler, this);
    _server.onDisconnect(onDisconnectHandler, this);

    String server_name = SettingsManager::get(STR_PREF_SERVER_SSID, STR_CHESS_GAME_DIR);
    String server_pwd = SettingsManager::get(STR_PREF_SERVER_PWD, STR_CHESS_GAME_DIR);

    if (server_name.isEmpty())
      server_name = STR_DEF_SSID;

    if (server_pwd.isEmpty())
      server_pwd = STR_DEF_PWD;

    _server.begin(STR_CHESS_GAME_ID, server_name, server_pwd, true, 3);
    _server.open();
  }

  ChessServerContext::~ChessServerContext()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  bool ChessServerContext::loop()
  {
    return true;
  }

  void ChessServerContext::update()
  {
    (this->*_state_input_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::showLobbyTmpl()
  {
    _state_input_handler = &ChessServerContext::handleLobbyInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* empty_lobby_msg = new Label(ID_EMPTY_MSG);
    layout->addWidget(empty_lobby_msg);

    empty_lobby_msg->setText(STR_MISSING_CLIENTS);
    empty_lobby_msg->setWidth(UI_WIDTH);
    empty_lobby_msg->setAutoscroll(true);
    empty_lobby_msg->setBackColor(layout->getBackColor());
    empty_lobby_msg->setPos(0, getCenterY(empty_lobby_msg));
    empty_lobby_msg->setAlign(IWidget::ALIGN_CENTER);
    empty_lobby_msg->setGravity(IWidget::GRAVITY_CENTER);

    const std::unordered_map<uint32_t, pixeler::ClientSession>* clients = _server.getClients();

    if (clients->empty())
      return;

    FixedMenu* clients_menu = new FixedMenu(ID_CLIENT_LIST);
    layout->addWidget(clients_menu);
    clients_menu->setBackColor(COLOR_MAIN_BACK);
    clients_menu->setWidth(UI_WIDTH - DISPLAY_PADDING * 2);
    clients_menu->setHeight(UI_HEIGHT);
    clients_menu->setItemHeight(UI_HEIGHT / 4 - 2);
    clients_menu->setPos(DISPLAY_PADDING, 0);

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

  void ChessServerContext::handleLobbyInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      openContext(new ChessContext());
    else if (_input.isPressed(BtnID::BTN_OK))
      showLobbyContextMenuTmpl();
    else if (_input.isReleased(BtnID::BTN_UP))
      showLobbyContextMenuTmpl();
    else if (_input.isReleased(BtnID::BTN_DOWN))
      showLobbyContextMenuTmpl();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::showLobbyContextMenuTmpl()
  {
    _state_input_handler = &ChessServerContext::handleContextMenuInput;

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

  void ChessServerContext::hideLobbyContextMenu()
  {
    _state_input_handler = &ChessServerContext::handleLobbyInput;

    getLayout()->delWidgetByID(ID_CONTEXT_MENU);

    IWidget* raw_menu = getLayout()->getWidgetByID(ID_CLIENT_LIST);
    if (raw_menu)
    {
      FixedMenu* clients_menu = raw_menu->castTo<FixedMenu>();
      clients_menu->enable();
    }
  }

  void ChessServerContext::handleContextMenuInput()
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
          break;
        case ID_ITEM_START_GAME:
          _server.close();
          // TODO start game
          break;
        case ID_ITEM_KICK_CLIENT:
        {
          FixedMenu* client_list = getLayout()->getWidgetByID(ID_CLIENT_LIST)->castTo<FixedMenu>();
          _server.removeClient(client_list->getCurrItemText());
        }
        break;

        default:
          break;
      }

      hideLobbyContextMenu();
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

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::showClientConfirmTmpl(String client_name)
  {
    _state_input_handler = &ChessServerContext::handleClientConfirmInput;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* confirm_title = new Label(ID_LBL_CONFIRM_TITLE);
    layout->addWidget(confirm_title);
    confirm_title->setText(STR_WANTS_TO_JOIN);
    confirm_title->setTextColor(COLOR_BLACK);
    confirm_title->setBackColor(COLOR_MAIN_BACK);
    confirm_title->setGravity(IWidget::GRAVITY_CENTER);
    confirm_title->setWidth(UI_WIDTH);
    confirm_title->setHeight(20);
    confirm_title->setAutoscroll(true);

    Label* name_lbl = confirm_title->clone(ID_LBL_CLIENT_NAME);
    layout->addWidget(name_lbl);
    name_lbl->setText(client_name);
    name_lbl->setPos(0, confirm_title->getBottomYPos() + 5);
    name_lbl->setBackColor(COLOR_GREEN);
    name_lbl->setFont(font_inr24);

    Label* confirm_way = confirm_title->clone(ID_LBL_CONFIRM_WAY);
    layout->addWidget(confirm_way);
    confirm_way->setText(STR_CONFIRM_WAY);
    confirm_way->setPos(0, name_lbl->getBottomYPos() + 5);

    Label* reject_way = confirm_title->clone(ID_LBL_REJECT_WAY);
    layout->addWidget(reject_way);
    reject_way->setText(STR_REJECT_WAY);
    reject_way->setPos(0, confirm_way->getBottomYPos() + 5);
  }

  void ChessServerContext::handleClientConfirmInput()
  {
    if (_input.isReleased(BtnID::BTN_BACK))
      handleClientConfirmResult(false);
    else if (_input.isReleased(BtnID::BTN_OK))
      handleClientConfirmResult(true);
  }

  void ChessServerContext::handleClientConfirmResult(bool is_accepted)
  {
    Label* client_name_lbl = getLayout()->getWidgetByID(ID_LBL_CLIENT_NAME)->castTo<Label>();
    _server.resolveJoin(client_name_lbl->getText(), is_accepted);
    showLobbyTmpl();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::onConfirmationHandler(const String client_name, void* arg)
  {
    ChessServerContext* self = static_cast<ChessServerContext*>(arg);
    self->post([self, client_name]()
               { self->showClientConfirmTmpl(client_name); }, 500);
  }

  void ChessServerContext::onDisconnectHandler(const String client_name, void* arg)
  {
    ChessServerContext* self = static_cast<ChessServerContext*>(arg);
    self->post([self]()
               { self->showLobbyTmpl(); }, 500);
  }

  //----------------------------------------------------------------------------------------------------------
}  // namespace chess
