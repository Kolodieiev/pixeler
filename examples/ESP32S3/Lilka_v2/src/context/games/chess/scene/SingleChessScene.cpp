#include "SingleChessScene.h"

namespace chess
{
  SingleChessScene::SingleChessScene(DataStream& stored_objs) : IChessScene(stored_objs, true)
  {
  }

  SingleChessScene::~SingleChessScene()
  {
  }

  void SingleChessScene::update()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
    {
      _is_finished = true;
    }
    if (_input.isHolded(BtnID::BTN_UP))
    {
      if (_board.isWhiteTurn())
        moveCursorUp();
      else
        moveCursorDown();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      if (_board.isWhiteTurn())
        moveCursorDown();
      else
        moveCursorUp();
    }
    else if (_input.isHolded(BtnID::BTN_RIGHT))
    {
      if (_board.isWhiteTurn())
        moveCursorRight();
      else
        moveCursorLeft();
    }
    else if (_input.isHolded(BtnID::BTN_LEFT))
    {
      if (_board.isWhiteTurn())
        moveCursorLeft();
      else
        moveCursorRight();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      handleOkClick();
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      clearCurrSelect();
    }

    IChessScene::update();
  }

  void SingleChessScene::onTriggered(uint16_t id)
  {
  }

}  // namespace chess
