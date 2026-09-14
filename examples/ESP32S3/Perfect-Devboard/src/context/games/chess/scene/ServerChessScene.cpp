#include "ServerChessScene.h"

#include "DataSubtypes.h"

namespace chess
{
  static const uint8_t PACK_RESEND_ATTEMPTS{20};

  ServerChessScene::ServerChessScene(DataStream& stored_objs,
                                     GameServer& server,
                                     const IPAddress& main_client_ip)
      : IChessScene(stored_objs, false, true),
        _server{server},
        MAIN_CLIENT_IP(main_client_ip)
  {
    subscribeServerHandlers();
    startGame();
  }

  ServerChessScene::~ServerChessScene()
  {
    stopGame();
  }

  void ServerChessScene::update()
  {
    IChessScene::update();
    _server.requestAcks();
    (this->*_state_handler)();
  }

  void ServerChessScene::onTriggered(uint16_t id)
  {
    // тригери в грі відсутні, тому обробник порожній
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::subscribeServerHandlers()
  {
    _server.onGameData(onDataHandler, this);
    _server.onDisconnect(onDisconnectHandler, this);
  }

  void ServerChessScene::unsubscribeServerHandlers()
  {
    _server.onGameData(nullptr, nullptr);
    _server.onDisconnect(nullptr, nullptr);
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::startGame()
  {
    // TODO додати плашку про очікування клієнтів

    _server.broadcastStartGame();
    _state_handler = &ServerChessScene::waitFirstConnect;
  }

  void ServerChessScene::stopGame()
  {
    unsubscribeServerHandlers();
    _server.broadcastStopGame();  // Запрошуємо спостерігачів до лоббі
    _is_released = true;
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::onDisconnectHandler(const IPAddress& client_ip, void* arg)
  {
    ServerChessScene* self = static_cast<ServerChessScene*>(arg);

    if (client_ip == self->MAIN_CLIENT_IP)
    {
      self->post(
          [self]()
          {
            self->stopGame();
          });
    }
  }

  void ServerChessScene::onDataHandler(const ClientSession& session, const UdpPacket& packet, void* arg)
  {
    ServerChessScene* self = static_cast<ServerChessScene*>(arg);

    const IPAddress ip = session.getIP();
    if (ip != self->MAIN_CLIENT_IP)
    {
      // Спостерігачі не повинні надсилати будь-які дані, окрім підтвердження пакетів
      // Основний клієнт не повинен ходити поза чергою
      log_e("Клієнт зламано:  [%s] ", session.getName());
      self->post(
          [self, ip]()
          {
            self->removeCorruptedClient(ip);
          });

      return;
    }

    const uint8_t subtype = packet.getSubtype();

    // Тут перевіряємо пакет на вміст будь-якиї даних
    // Окрім ігрових.
    if (subtype == SUBTYPE_MAIN_CLIENT)
    {
      self->post(
          [self]()
          { self->_have_main_connect = true; });

      return;
    }

    // Клієнт не повинен надсилати ігрові дані,
    // якщо не настала його черга
    if (self->_board.isWhiteTurn())
      return;

    switch (subtype)
    {
      case SUBTYPE_MOVE_U:
        self->post(
            [self]()
            { self->moveUp(); });
        break;

      case SUBTYPE_MOVE_D:
        self->post(
            [self]()
            { self->moveDown(); });
        break;

      case SUBTYPE_MOVE_L:
        self->post(
            [self]()
            { self->moveLeft(); });
        break;

      case SUBTYPE_MOVE_R:
        self->post(
            [self]()
            { self->moveRight(); });
        break;

      case SUBTYPE_MOVE_OK:
        self->post(
            [self]()
            { self->moveOk(); });
        break;

      case SUBTYPE_CLEAR_SELECT:
        self->post(
            [self]()
            { self->clearSelect(); });
        break;

      default:
        log_e("Невідомий підтип пакета: %u", subtype);
        break;
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::moveUp()
  {
    if (!_server.isAllAcked())
      return;

    broadcastAction(SUBTYPE_MOVE_U);
    moveCursorUp();
  }

  void ServerChessScene::moveDown()
  {
    broadcastAction(SUBTYPE_MOVE_D);
    moveCursorDown();
  }

  void ServerChessScene::moveLeft()
  {
    broadcastAction(SUBTYPE_MOVE_L);
    moveCursorLeft();
  }

  void ServerChessScene::moveRight()
  {
    broadcastAction(SUBTYPE_MOVE_R);
    moveCursorRight();
  }

  void ServerChessScene::moveOk()
  {
    broadcastAction(SUBTYPE_MOVE_OK);
    handleOkClick();
  }

  void ServerChessScene::clearSelect()
  {
    broadcastAction(SUBTYPE_CLEAR_SELECT);
    clearCurrSelect();
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::removeCorruptedClient(const IPAddress& client_ip)
  {
    _server.removeSession(client_ip);
  }

  //----------------------------------------------------------------------------------------------------------

  void ServerChessScene::waitFirstConnect()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
    {
      stopGame();
      return;
    }

    if (!_server.isAllAcked())
      return;

    if (!_have_main_connect)
    {
      // додатковий пакет синхронізації, для очікування завантаження клієнтом ігрової сцени
      _server.send(MAIN_CLIENT_IP, UdpPacket::TYPE_GAME_DATA, SUBTYPE_MAIN_CLIENT);
      return;
    }

    // TODO прибрати плашку про очікування клієнтів
    _state_handler = &ServerChessScene::handleGameInput;
  }

  void ServerChessScene::handleGameInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
    {
      stopGame();
      return;
    }

    if (!_board.isWhiteTurn())
      return;

    if (_input.isHolded(BtnID::BTN_UP))
      moveUp();
    else if (_input.isHolded(BtnID::BTN_DOWN))
      moveDown();
    else if (_input.isHolded(BtnID::BTN_RIGHT))
      moveRight();
    else if (_input.isHolded(BtnID::BTN_LEFT))
      moveLeft();
    else if (_input.isReleased(BtnID::BTN_OK))
      moveOk();
    else if (_input.isReleased(BtnID::BTN_BACK))
      clearSelect();
  }

  void ServerChessScene::broadcastAction(uint8_t subtype)
  {
    _server.broadcast(UdpPacket::TYPE_GAME_DATA, subtype);
  }

  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
