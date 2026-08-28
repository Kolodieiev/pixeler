#include "ChessClientContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "widget/menu/FixedMenu.h"
#include "widget/text/TextBox.h"

namespace chess
{
  static const char STR_SERVER_SCANNING[] = "Зачекайте, відбувається сканування";
  static const char STR_SERVER_SCANNING_DONE[] = "Сканування завершено";

  static const char STR_SELECT_SERVER[] = "Оберіть сервер";

  static const char STR_ENTER_PWD[] = "Введіть пароль до: ";
  static const char STR_CONNECTING_TO[] = "Очікуємо підключення до: ";

  static const char STR_SERVER_UNAVAILABLE[] = "Сервер не відповідає";
  static const char STR_CONNECTING_ERROR[] = "Помилка підключення";
  static const char STR_CONNECTING[] = "Підключення до сервера";

  static const char STR_WAITING_GAME[] = "Очікуйте запуск гри";
  static const char STR_DISCONNECTED[] = "Від'єднано від сервера";

  //----------------------------------------------------------------------------------------------------------

  ChessClientContext::ChessClientContext()
  {
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
    (this->*_state_kb_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showConnDialogTmpl()
  {
    // _state_kb_handler = &ChessContext::procDialog;
    // addDialog(STR_NICKNAME, _client_nick);  // TODO тут відображати імя SSID
  }

  //----------------------------------------------------------------------------------------------------------
}  // namespace chess
