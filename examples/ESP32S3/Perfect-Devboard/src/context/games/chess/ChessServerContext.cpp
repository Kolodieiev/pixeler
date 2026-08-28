#include "ChessServerContext.h"

#include "../../WidgetCreator.h"
#include "ChessContext.h"
#include "widget/menu/FixedMenu.h"

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
    showLobbyTmpl();
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
    (this->*_state_kb_handler)();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::showLobbyTmpl()
  {
    _state_kb_handler = &ChessServerContext::procLobbyKb;

    EmptyLayout* layout = WidgetCreator::getEmptyLayout();
    setLayout(layout);

    Label* empty_lobby_msg = new Label(1);
    layout->addWidget(empty_lobby_msg);

    empty_lobby_msg->setText(STR_WAITING_CLIENT);
    empty_lobby_msg->setWidth(UI_WIDTH);
    empty_lobby_msg->setAutoscroll(true);
    empty_lobby_msg->setBackColor(layout->getBackColor());
    empty_lobby_msg->setPos(0, getCenterY(empty_lobby_msg));
    empty_lobby_msg->setAlign(IWidget::ALIGN_CENTER);
    empty_lobby_msg->setGravity(IWidget::GRAVITY_CENTER);
  }

  void ChessServerContext::procLobbyKb()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
      openContext(new ChessContext());
    else if (_input.isPressed(BtnID::BTN_OK))
      showContextMenuTmpl();
    else if (_input.isReleased(BtnID::BTN_UP))
      showContextMenuTmpl();
    else if (_input.isReleased(BtnID::BTN_DOWN))
      showContextMenuTmpl();
  }

  //----------------------------------------------------------------------------------------------------------

  void ChessServerContext::showContextMenuTmpl()
  {
  }

  //----------------------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------------------

}  // namespace chess
