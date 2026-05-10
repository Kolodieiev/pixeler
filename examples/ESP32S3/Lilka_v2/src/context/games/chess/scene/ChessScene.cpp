#include "ChessScene.h"

#include "../obj/IPiece.h"
#include "../obj/bishop/BishopObj.h"
#include "../obj/king/KingObj.h"
#include "../obj/knight/KnightObj.h"
#include "../obj/pawn/PawnObj.h"
#include "../obj/queen/QueenObj.h"
#include "../obj/rook/RookObj.h"
#include "../res/board_img.h"

namespace chess
{
  static const char STR_WHITE_TURN[] = "Хід білих";
  static const char STR_BLACK_TURN[] = "Хід чорних";
  static const char STR_BLACK_WIN[] = "Чорні перемогли!";
  static const char STR_WHITE_WIN[] = "Білі перемогли!";
  static const char STR_STALEMATE[] = "Нічия!";

  static const uint16_t BOARD_IMG_GLOB_OFF = (UI_HEIGHT - SPRITE_CHESS_BOARD_SZ) / 2;
  static const uint16_t BOARD_IMG_OFF_X = 8;
  static const uint16_t BOARD_IMG_OFF_Y = 8;
  static const uint16_t BOARD_SQUARE_SZ = 28;
  static const uint16_t BOARD_PIECE_SZ = 26;

  ChessScene::ChessScene(DataStream& stored_objs, uint8_t players_num) : IGameScene(stored_objs),
                                                                         _GAME_MODE{players_num > 1 ? GAME_MODE_TWO_PL : GAME_MODE_ONE_PL},
                                                                         _board{0, BOARD_IMG_GLOB_OFF,
                                                                                BOARD_IMG_OFF_X,
                                                                                BOARD_IMG_OFF_Y + BOARD_IMG_GLOB_OFF,
                                                                                BOARD_SQUARE_SZ, BOARD_PIECE_SZ},
                                                                         _cur_y{BOARD_IMG_GLOB_OFF}
  {
    _msg_lbl = new Label(1);
    _msg_lbl->setWidth(UI_WIDTH);
    _msg_lbl->setAlign(IWidget::ALIGN_CENTER);
    _msg_lbl->setGravity(IWidget::GRAVITY_CENTER);
    _msg_lbl->setFont(font_10x20);
    _msg_lbl->setBackColor(COLOR_GREY);
    _msg_lbl->setPos(0, 10);

    buildTerrain();
    createMainObj();
    prepareBoard();
  }

  ChessScene::~ChessScene()
  {
    delete _msg_lbl;
    delete _main_obj;
  }

  void ChessScene::update()
  {
    if (_input.isPressed(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, PRESS_LOCK);
      // Оброблювати клієнт-серверну поведінку
      _is_finished = true;
    }
    if (_input.isHolded(BtnID::BTN_UP))
    {
      _input.lock(BtnID::BTN_UP, HOLD_LOCK);
      if (_board.isWhiteTurn())
        moveCursorUp();
      else
        moveCursorDown();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
      if (_board.isWhiteTurn())
        moveCursorDown();
      else
        moveCursorUp();
    }
    else if (_input.isHolded(BtnID::BTN_RIGHT))
    {
      _input.lock(BtnID::BTN_RIGHT, HOLD_LOCK);
      if (_board.isWhiteTurn())
        moveCursorRight();
      else
        moveCursorLeft();
    }
    else if (_input.isHolded(BtnID::BTN_LEFT))
    {
      _input.lock(BtnID::BTN_LEFT, HOLD_LOCK);
      if (_board.isWhiteTurn())
        moveCursorLeft();
      else
        moveCursorRight();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      _input.lock(BtnID::BTN_OK, CLICK_LOCK);
      handleOkClick();
    }
    else if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      clearCurrSelect();
    }

    IGameScene::update();

    // Малюємо курсор після оновлення сцени, щоб фон не перемальовував його
    if (_is_piece_selected)
    {
      // Фігуру обрано, малюємо можливі ходи
      for (const auto& pos : _possible_moves)
        _display.drawRoundRect(pos.x + BOARD_IMG_OFF_X, pos.y + BOARD_IMG_OFF_Y, BOARD_SQUARE_SZ, BOARD_SQUARE_SZ, 5, COLOR_GREEN);
    }

    // Малюємо курсор
    _display.drawRoundRect(_cur_x + BOARD_IMG_OFF_X, _cur_y + BOARD_IMG_OFF_Y, BOARD_SQUARE_SZ, BOARD_SQUARE_SZ, 5, COLOR_RED);

    // Повертаємо дошку
    if (!_board.isWhiteTurn())  // TODO
    {
      _display.rotateDisplaySquare(0, BOARD_IMG_GLOB_OFF, SPRITE_CHESS_BOARD_SZ, DisplayWrapper::ROTATE_ANGLE_180);
    }

    // TODO перенести в UI
    if (_board.isCheckmate())
    {
      if (_board.isWhiteTurn())  // TODO обробка сервера.
        _msg_lbl->setText(STR_BLACK_WIN);
      else
        _msg_lbl->setText(STR_WHITE_WIN);
    }
    else if (_board.isStalemate())
    {
      _msg_lbl->setText(STR_STALEMATE);
    }
    else if (_board.isWhiteTurn())
    {
      _msg_lbl->setText(STR_WHITE_TURN);
    }
    else
    {
      _msg_lbl->setText(STR_BLACK_TURN);
    }

    _msg_lbl->drawForced();
  }

  void ChessScene::moveCursorUp()
  {
    if (_cur_y - BOARD_SQUARE_SZ < BOARD_IMG_GLOB_OFF)
      _cur_y = SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_Y * 2 + BOARD_IMG_GLOB_OFF - BOARD_SQUARE_SZ;
    else
      _cur_y -= BOARD_SQUARE_SZ;
  }

  void ChessScene::moveCursorDown()
  {
    if (_cur_y + BOARD_SQUARE_SZ < SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_Y * 2 + BOARD_IMG_GLOB_OFF)
      _cur_y += BOARD_SQUARE_SZ;
    else
      _cur_y = BOARD_IMG_GLOB_OFF;
  }

  void ChessScene::moveCursorLeft()
  {
    if (_cur_x - BOARD_SQUARE_SZ < 0)
      _cur_x = SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_X * 2 - BOARD_SQUARE_SZ;
    else
      _cur_x -= BOARD_SQUARE_SZ;
  }

  void ChessScene::moveCursorRight()
  {
    if (_cur_x + BOARD_SQUARE_SZ < SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_X * 2)
      _cur_x += BOARD_SQUARE_SZ;
    else
      _cur_x = 0;
  }

  void ChessScene::handleOkClick()
  {
    if (!_is_piece_selected)  // Якщо ще не обрано жодну фігуру
    {
      _possible_moves = _board.getPossibleMoves(_cur_x, _cur_y);
      if (!_possible_moves.empty())
      {
        _is_piece_selected = true;
        _cur_x_selected = _cur_x;  // Запам'ятати де натиснули "вибір"
        _cur_y_selected = _cur_y;
      }
    }
    else
    {
      for (const auto& pos : _possible_moves)
      {
        if (pos.x == _cur_x && pos.y == _cur_y)
        {
          _board.movePiece(_cur_x_selected, _cur_y_selected, _cur_x, _cur_y);
          clearCurrSelect();
          break;
        }
      }
    }
  }

  void ChessScene::clearCurrSelect()
  {
    _is_piece_selected = false;
    _cur_x_selected = 0;
    _cur_y_selected = 0;
  }

  void ChessScene::onTriggered(uint16_t id)
  {
  }

  void ChessScene::buildTerrain()
  {
    _terrain.setBackImg(SPRITE_CHESS_BOARD, SPRITE_CHESS_BOARD_SZ, SPRITE_CHESS_BOARD_SZ, COLOR_GREY, 0, BOARD_IMG_GLOB_OFF);
  }

  void ChessScene::createMainObj()
  {
    _camera = createObject<CameraObj>();
    _main_obj = _camera;
  }

  void ChessScene::prepareBoard()
  {
    // const uint8_t SPRITE_SIDE_LEN = 26;
    // const uint8_t START_OFFSET = 9;
    // uint16_t x_pos = START_OFFSET;
    // uint16_t y_pos = START_OFFSET + SPRITE_SIDE_LEN;
    // //
    // x_pos += SPRITE_SIDE_LEN + 2;

    // pawn->setPos(x_pos, y_pos);

    // Bishop
    for (int i = 0; i < 2; ++i)
    {
      BishopObj* bish = createObject<BishopObj>();
      addObject(*bish);
      _board.addPiece(bish);
    }

    for (int i = 0; i < 2; ++i)
    {
      BishopObj* bish = createObject<BishopObj>();
      addObject(*bish);
      _board.addPiece(bish);
      bish->setIsWhite(false);
    }

    // King
    KingObj* king_w = createObject<KingObj>();
    addObject(*king_w);
    _board.addPiece(king_w);

    KingObj* king_b = createObject<KingObj>();
    addObject(*king_b);
    _board.addPiece(king_b);
    king_b->setIsWhite(false);

    // Knight
    for (int i = 0; i < 2; ++i)
    {
      KnightObj* knight = createObject<KnightObj>();
      addObject(*knight);
      _board.addPiece(knight);
    }

    for (int i = 0; i < 2; ++i)
    {
      KnightObj* knight = createObject<KnightObj>();
      addObject(*knight);
      _board.addPiece(knight);
      knight->setIsWhite(false);
    }

    // Queen
    QueenObj* queen_w = createObject<QueenObj>();
    addObject(*queen_w);
    _board.addPiece(queen_w);

    QueenObj* queen_b = createObject<QueenObj>();
    addObject(*queen_b);
    _board.addPiece(queen_b);
    queen_b->setIsWhite(false);

    // Rook
    for (int i = 0; i < 2; ++i)
    {
      RookObj* rook = createObject<RookObj>();
      addObject(*rook);
      _board.addPiece(rook);
    }

    for (int i = 0; i < 2; ++i)
    {
      RookObj* rook = createObject<RookObj>();
      addObject(*rook);
      _board.addPiece(rook);
      rook->setIsWhite(false);
    }

    // Pawn
    for (int i = 0; i < 8; ++i)
    {
      PawnObj* pawn = createObject<PawnObj>();
      addObject(*pawn);
      _board.addPiece(pawn);
    }

    for (int i = 0; i < 8; ++i)
    {
      PawnObj* pawn = createObject<PawnObj>();
      addObject(*pawn);
      _board.addPiece(pawn);
      pawn->setIsWhite(false);
    }

    _board.reset();
  }
}  // namespace chess
