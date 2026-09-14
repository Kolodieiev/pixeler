#include "ClientChessScene.h"

#include "DataSubtypes.h"

namespace chess
{
  ClientChessScene::ClientChessScene(DataStream& stored_objs,
                                     GameClient& client)
      : IChessScene(stored_objs, false, false),
        _client{client}
  {
    startGame();
  }

  ClientChessScene::~ClientChessScene()
  {
  }

  void ClientChessScene::update()
  {
    IChessScene::update();
    (this->*_state_handler)();
  }

  void ClientChessScene::onTriggered(uint16_t id)
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessScene::onDisconnectHandler(void* arg)
  {
    ClientChessScene* self = static_cast<ClientChessScene*>(arg);

    self->post(
        [self]()
        {
          self->stopGame();
        });
  }

  void ClientChessScene::onGameStartHandler(void* arg)
  {
    // Якщо сервер не отримав пакет підтвердження, він повторно надішле команду запуску гри
    // Клієнт надішле ACK у відповідь повторно автоматично
  }

  void ClientChessScene::onGameStopHandler(void* arg)
  {
    // Відписуємось на місці від прослуховування,
    // щоб сервер не міг викликати підписники в закритій сцені
    ClientChessScene* self = static_cast<ClientChessScene*>(arg);
    self->stopGame();
  }

  void ClientChessScene::onDataHandler(const UdpPacket& packet, void* arg)
  {
    ClientChessScene* self = static_cast<ClientChessScene*>(arg);

    const uint8_t subtype = packet.getSubtype();
    switch (subtype)
    {
      case SUBTYPE_MOVE_U:
        self->post(
            [self]()
            { self->moveCursorUp(); });
        break;

      case SUBTYPE_MOVE_D:
        self->post(
            [self]()
            { self->moveCursorDown(); });
        break;

      case SUBTYPE_MOVE_L:
        self->post(
            [self]()
            { self->moveCursorLeft(); });
        break;

      case SUBTYPE_MOVE_R:
        self->post(
            [self]()
            { self->moveCursorRight(); });
        break;

      case SUBTYPE_MOVE_OK:
        self->post(
            [self]()
            { self->handleOkClick(); });
        break;

      case SUBTYPE_CLEAR_SELECT:
        self->post(
            [self]()
            { self->clearCurrSelect(); });
        break;

      case SUBTYPE_MAIN_CLIENT:
        self->post(
            [self]()
            {
              self->_is_observer = false;
              self->_client.send(UdpPacket::TYPE_GAME_DATA, SUBTYPE_MAIN_CLIENT);
            });
        break;

      default:
        log_e("Невідомий підтип пакета: %u", subtype);
        break;
    }
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessScene::startGame()
  {
    subscribeClientHandlers();
    _state_handler = &ClientChessScene::handleGameInput;
  }

  void ClientChessScene::stopGame()
  {
    unsubscribeClientHandlers();
    _is_released = true;
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessScene::subscribeClientHandlers()
  {
    _client.onDisconnect(onDisconnectHandler, this);
    _client.onGameData(onDataHandler, this);
    _client.onGameStart(onGameStartHandler, this);
    _client.onGameStop(onGameStopHandler, this);
  }

  void ClientChessScene::unsubscribeClientHandlers()
  {
    _client.onDisconnect(nullptr, nullptr);
    _client.onGameData(nullptr, nullptr);
    _client.onGameStart(nullptr, nullptr);
    _client.onGameStop(nullptr, nullptr);
  }

  //----------------------------------------------------------------------------------------------------------

  void ClientChessScene::handleGameInput()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
    {
      stopGame();  // TODO відключатися повністю якщо виходимо вручну або надіслати пакет про вихід
      return;
    }

    if (_is_observer)
      return;

    if (_board.isWhiteTurn())
      return;

    if (_input.isHolded(BtnID::BTN_UP))
      sendAction(SUBTYPE_MOVE_D);
    else if (_input.isHolded(BtnID::BTN_DOWN))
      sendAction(SUBTYPE_MOVE_U);
    else if (_input.isHolded(BtnID::BTN_RIGHT))
      sendAction(SUBTYPE_MOVE_L);
    else if (_input.isHolded(BtnID::BTN_LEFT))
      sendAction(SUBTYPE_MOVE_R);
    else if (_input.isReleased(BtnID::BTN_OK))
      sendAction(SUBTYPE_MOVE_OK);
    else if (_input.isReleased(BtnID::BTN_BACK))
      sendAction(SUBTYPE_CLEAR_SELECT);
  }

  void ClientChessScene::sendAction(uint8_t subtype)
  {
    _client.send(UdpPacket::TYPE_GAME_DATA, subtype);
  }

  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
