#include "ChessClientContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "manager/SettingsManager.h"

namespace chess
{
  //----------------------------------------------------------------------------------------------------------

  ChessClientContext::ChessClientContext()
  {
    // _client.connect(); TODO
    setupWiFi();
    showAPScanTmpl();
  }

  ChessClientContext::~ChessClientContext()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  bool ChessClientContext::loop()
  {
    return true;
  }

  void ChessClientContext::update()
  {
    (this->*_state_input_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::setupWiFi()
  {
    _wifi.enable();

  }

  void ChessClientContext::showAPScanTmpl()
  {
  }

  void ChessClientContext::handleAPScanInput()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showAPListTmpl()
  {
  }

  void ChessClientContext::handleAPListInput()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showAPConnectTmpl()
  {
    // TODO show connect dialog
    // TODO save pwd if success
  }

  void ChessClientContext::handleAPConnectInput()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showLobbyTmpl()
  {
  }

  void ChessClientContext::handleLobbyInput()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showConnectDialogTmpl()
  {
    // _state_input_handler = &ChessContext::handleDialogInput;
    // addDialog(STR_NICKNAME, _client_nick);  // TODO тут відображати імя SSID
  }

  void ChessClientContext::handleConnectDialogInput()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::onConnectHandler(void* arg)
  {
  }

  void ChessClientContext::onDisconnectHandler(void* arg)
  {
  }

  //----------------------------------------------------------------------------------------------------------
}  // namespace chess
