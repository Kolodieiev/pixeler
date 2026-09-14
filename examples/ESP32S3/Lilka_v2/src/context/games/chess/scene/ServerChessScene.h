#pragma once
#include "IChessScene.h"
#include "game/online/GameServer.h"

namespace chess
{
  class ServerChessScene : public IChessScene
  {
  public:
    explicit ServerChessScene(DataStream& stored_objs, GameServer& server, const IPAddress& main_client_ip);
    virtual ~ServerChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) override;

  private:
    using StateHandler = void (ServerChessScene::*)();

    void subscribeServerHandlers();
    void unsubscribeServerHandlers();

    void startGame();
    void stopGame();

    static void onDisconnectHandler(const IPAddress& client_ip, void* arg);
    static void onDataHandler(const ClientSession& session, const UdpPacket& packet, void* arg);

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void moveOk();
    void clearSelect();

    void removeCorruptedClient(const IPAddress& client_ip);

    void waitFirstConnect();
    void handleGameInput();

    void broadcastAction(uint8_t subtype);

  private:
    GameServer& _server;
    const IPAddress MAIN_CLIENT_IP;
    StateHandler _state_handler{nullptr};

    bool _have_main_connect{false};
  };

}  // namespace chess
