#pragma once
#include "IChessScene.h"
#include "game/online/GameClient.h"

namespace chess
{
  class ClientChessScene : public IChessScene
  {
  public:
    explicit ClientChessScene(DataStream& stored_objs, GameClient& client);
    virtual ~ClientChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) override;

  private:
    using StateHandler = void (ClientChessScene::*)();

    void subscribeClientHandlers();
    void unsubscribeClientHandlers();

    void startGame();
    void stopGame();

    static void onDisconnectHandler(void* arg);
    static void onDataHandler(const UdpPacket& packet, void* arg);
    static void onGameStartHandler(void* arg);
    static void onGameStopHandler(void* arg);

    void handleGameInput();
    void sendAction(uint8_t subtype);

  private:
    GameClient& _client;

    StateHandler _state_handler{nullptr};

    bool _is_observer{true};
  };
}  // namespace chess
