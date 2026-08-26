#include "ChessServerContext.h"

#include "ChessContext.h"

namespace chess
{
  static const char STR_WAITING_CLIENT[] = "Очікуйте приєднання клієнтів";
  static const char STR_WANTS_TO_JOIN[] = " хоче приєднатися";

  static const char STR_CONT_DISC_CLIENT[] = "Відключити клієнта";
  static const char STR_CONT_OPEN_LOBBY[] = "Відкрити лоббі";
  static const char STR_CONT_CLOSE_LOBBY[] = "Закрити лоббі";
  static const char STR_CONT_GAME_START[] = "Розпочати гру";
  static const char STR_CONT_GAME_EXIT[] = "Завершити гру";

  //----------------------------------------------------------------------------------------------------------

  ChessServerContext::ChessServerContext()
  {
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
    (this->*_current_state)();
  }

  //----------------------------------------------------------------------------------------------------------


  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
