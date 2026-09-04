#include "IChessScene.h"

#include "../obj/IPiece.h"
#include "../obj/TypeID.h"
#include "../obj/bishop/BishopObj.h"
#include "../obj/bishop/bishop_img.h"
#include "../obj/king/KingObj.h"
#include "../obj/king/king_img.h"
#include "../obj/knight/KnightObj.h"
#include "../obj/knight/knight_img.h"
#include "../obj/pawn/PawnObj.h"
#include "../obj/pawn/pawn_img.h"
#include "../obj/queen/QueenObj.h"
#include "../obj/queen/queen_img.h"
#include "../obj/rook/RookObj.h"
#include "../obj/rook/rook_img.h"
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

  IChessScene::IChessScene(DataStream& stored_objs, bool is_white) : IGameScene2D(stored_objs),
                                                                     IS_WHITE{is_white},
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

    createSpiteTmpls();
    buildTerrain();
    createMainObj();
    prepareBoard();
  }

  IChessScene::~IChessScene()
  {
    delete _msg_lbl;
    delete _main_obj;
  }

  void IChessScene::update()
  {
    IGameScene2D::update();

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
    if (!_board.isWhiteTurn())
    {
      _display.rotateDisplaySquare(0, BOARD_IMG_GLOB_OFF, SPRITE_CHESS_BOARD_SZ, DisplayWrapper::ROTATE_ANGLE_180);
    }

    // TODO перенести в UI
    if (_board.isCheckmate())
    {
      if (_board.isWhiteTurn())
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

  void IChessScene::buildTerrain()
  {
    _terrain.setBackImg(SPRITE_CHESS_BOARD, SPRITE_CHESS_BOARD_SZ, SPRITE_CHESS_BOARD_SZ, COLOR_GREY, 0, BOARD_IMG_GLOB_OFF);
  }

  void IChessScene::createMainObj()
  {
    _camera = createObject<CameraObj>();
    _main_obj = _camera;
  }

  void IChessScene::prepareBoard()  // TODO is_white
  {
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

  void IChessScene::createSpiteTmpls()
  {
    SpriteGeometry sprite_geometry{
        .rigid_offsets{},
        .width = 26,
        .height = sprite_geometry.width,
        .x_pivot = static_cast<uint16_t>(sprite_geometry.width / 2),
        .y_pivot = static_cast<uint16_t>(sprite_geometry.height / 2),
    };

    SpriteTemplate none_tmpl{};
    registerSpriteTemplate(TYPE_NONE, none_tmpl);
    //
    SpriteTemplate bishop_tmpl{};
    bishop_tmpl.img_variants.push_back(SPRITE_BISHOP_BLACK);
    bishop_tmpl.img_variants.push_back(SPRITE_BISHOP_WHITE);
    bishop_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_BISHOP, bishop_tmpl);
    //
    SpriteTemplate king_tmpl{};
    king_tmpl.img_variants.push_back(SPRITE_KING_BLACK);
    king_tmpl.img_variants.push_back(SPRITE_KING_WHITE);
    king_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_KING, king_tmpl);
    //
    SpriteTemplate knight_tmpl{};
    knight_tmpl.img_variants.push_back(SPRITE_KNIGHT_BLACK);
    knight_tmpl.img_variants.push_back(SPRITE_KNIGHT_WHITE);
    knight_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_KNIGHT, knight_tmpl);
    //
    SpriteTemplate pawn_tmpl{};
    pawn_tmpl.img_variants.push_back(SPRITE_PAWN_BLACK);
    pawn_tmpl.img_variants.push_back(SPRITE_PAWN_WHITE);
    pawn_tmpl.img_variants.push_back(SPRITE_QUEEN_BLACK);
    pawn_tmpl.img_variants.push_back(SPRITE_QUEEN_WHITE);
    pawn_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_PAWN, pawn_tmpl);
    //
    SpriteTemplate player_tmpl{};
    registerSpriteTemplate(TYPE_PLAYER, player_tmpl);
    //
    SpriteTemplate queen_tmpl{};
    queen_tmpl.img_variants.push_back(SPRITE_QUEEN_BLACK);
    queen_tmpl.img_variants.push_back(SPRITE_QUEEN_WHITE);
    queen_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_QUEEN, queen_tmpl);
    //
    SpriteTemplate rook_tmpl{};
    rook_tmpl.img_variants.push_back(SPRITE_ROOK_BLACK);
    rook_tmpl.img_variants.push_back(SPRITE_ROOK_WHITE);
    rook_tmpl.geometry_variants.push_back(sprite_geometry);
    registerSpriteTemplate(TYPE_ROOK, rook_tmpl);
  }

  void IChessScene::moveCursorUp()
  {
    if (_cur_y - BOARD_SQUARE_SZ < BOARD_IMG_GLOB_OFF)
      _cur_y = SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_Y * 2 + BOARD_IMG_GLOB_OFF - BOARD_SQUARE_SZ;
    else
      _cur_y -= BOARD_SQUARE_SZ;
  }

  void IChessScene::moveCursorDown()
  {
    if (_cur_y + BOARD_SQUARE_SZ < SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_Y * 2 + BOARD_IMG_GLOB_OFF)
      _cur_y += BOARD_SQUARE_SZ;
    else
      _cur_y = BOARD_IMG_GLOB_OFF;
  }

  void IChessScene::moveCursorLeft()
  {
    if (_cur_x - BOARD_SQUARE_SZ < 0)
      _cur_x = SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_X * 2 - BOARD_SQUARE_SZ;
    else
      _cur_x -= BOARD_SQUARE_SZ;
  }

  void IChessScene::moveCursorRight()
  {
    if (_cur_x + BOARD_SQUARE_SZ < SPRITE_CHESS_BOARD_SZ - BOARD_IMG_OFF_X * 2)
      _cur_x += BOARD_SQUARE_SZ;
    else
      _cur_x = 0;
  }

  void IChessScene::handleOkClick()
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

  void IChessScene::clearCurrSelect()
  {
    _is_piece_selected = false;
    _cur_x_selected = 0;
    _cur_y_selected = 0;
  }

}  // namespace chess
