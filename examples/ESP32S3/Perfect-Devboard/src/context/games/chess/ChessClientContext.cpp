#include "ChessClientContext.h"

#include "ChessContext.h"

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
    (this->*_current_state)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessClientContext::showConnDialogTmpl()
  {
    // _current_state = &ChessContext::procDialog;
    // addDialog(STR_NICKNAME, _client_nick);  // TODO тут відображати імя SSID
  }

  //----------------------------------------------------------------------------------------------------------
}  // namespace chess
