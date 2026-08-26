#pragma once
#include "context/IContext.h"

namespace chess
{
  class ChessContext : public pixeler::IContext
  {
  public:
    ChessContext();
    virtual ~ChessContext();

  protected:
    virtual bool loop() override;
    virtual void update() override;

  private:
    void showMainTmpl();  // Формування шаблону GUI головного меню
    void procMainMenu();  // Обробка вводу головного меню

  private:
    using StateHandler = void (ChessContext::*)();

    enum WidgetID : uint8_t
    {
      ID_MAIN_MENU = 1,
    };

    enum ItemID : uint8_t
    {
      ID_ITEM_ONE_PLAYER = 1,
      ID_ITEM_TWO_PLAYERS,
      ID_ITEM_CLIENT,
      ID_ITEM_SERVER,
      ID_ITEM_PREFS,
    };

    StateHandler _current_state{&ChessContext::procMainMenu};  // Змінна для збереження методу, який відповідає за обробку поточного стану контексту
  };
}  // namespace chess
