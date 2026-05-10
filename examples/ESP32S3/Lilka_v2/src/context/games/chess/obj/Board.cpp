#include "Board.h"

#include "IPiece.h"
#include "king/KingObj.h"
#include "pawn/PawnObj.h"
#include "rook/RookObj.h"

namespace chess
{
  Board::Board(uint16_t x_glob_offset,
               uint16_t y_glob_offset,
               uint16_t x_loc_offset,
               uint16_t y_loc_offset,
               uint16_t square_size,
               uint16_t piece_size) : _X_GLOB_OFFSET{x_glob_offset},
                                      _Y_GLOB_OFFSET{y_glob_offset},
                                      _X_LOC_OFFSET{x_loc_offset},
                                      _Y_LOC_OFFSET{y_loc_offset},
                                      _SQUARE_SIZE{square_size},
                                      _PIECE_SIZE{piece_size}

  {
  }

  Board::~Board()
  {
  }

  void Board::addPiece(IPiece* piece)
  {
    _pieces.push_back(piece);
  }

  void Board::reset()
  {
    _is_white_turn = true;
    _is_checkmate = false;
    _is_stalemate = false;

    for (int r = 0; r < 8; ++r)
    {
      for (int c = 0; c < 8; ++c)
      {
        _board[r][c] = nullptr;
      }
    }

    // Вектор для відстеження використаних об'єктів з вектора _pieces
    std::vector<bool> used(_pieces.size(), false);

    const TypeID backline[] = {
        TYPE_ROOK, TYPE_KNIGHT, TYPE_BISHOP, TYPE_KING, TYPE_QUEEN, TYPE_BISHOP, TYPE_KNIGHT, TYPE_ROOK};

    for (uint32_t col = 0; col < 8; ++col)
    {
      // Чорні
      IPiece* black_major = findFreePiece(backline[col], false, used);
      if (black_major)
      {
        _board[0][col] = black_major;
        updateObjPhysPos(black_major, 0, col);
      }
      // Пішаки
      IPiece* black_pawn = findFreePiece(TYPE_PAWN, false, used);
      if (black_pawn)
      {
        _board[1][col] = black_pawn;
        updateObjPhysPos(black_pawn, 1, col);
      }

      // Білі
      IPiece* white_major = findFreePiece(backline[col], true, used);
      if (white_major)
      {
        _board[7][col] = white_major;
        updateObjPhysPos(white_major, 7, col);
      }
      // Пішаки
      IPiece* white_pawn = findFreePiece(TYPE_PAWN, true, used);
      if (white_pawn)
      {
        _board[6][col] = white_pawn;
        updateObjPhysPos(white_pawn, 6, col);
      }
    }
  }

  std::vector<Position> Board::getPossibleMoves(uint16_t x, uint16_t y) const
  {
    Position pos = convertToGridPos(x, y);
    IPiece* piece = _board[pos.y][pos.x];

    if (piece == nullptr || piece->isWhite() != _is_white_turn)
      return {};

    std::vector<Position> logical_moves = piece->getPossibleMoves(*this);
    std::vector<Position> legal_moves;

    Position king_pos = getKingPos(_is_white_turn);
    bool is_king = (piece->getTypeID() == TYPE_KING);

    for (const auto& move : logical_moves)
    {
      IPiece* original_to = _board[move.y][move.x];

      auto rollback = [&]()
      {
        _board[pos.y][pos.x] = piece;
        _board[move.y][move.x] = original_to;
      };

      // Симулюємо хід у масиві
      _board[move.y][move.x] = piece;
      _board[pos.y][pos.x] = nullptr;

      // Визначаємо, яку точку перевіряти на атаку
      // Якщо ходимо королем - перевіряємо його нову позицію
      // Якщо іншою фігурою - використовуємо заздалегідь знайдену king_pos
      Position check_pos = is_king ? move : king_pos;

      // Перевірка рокіровки
      if (is_king && __builtin_abs(static_cast<int>(move.x) - static_cast<int>(pos.x)) > 1)
      {
        // Первірка чи ходив король
        KingObj* king_castlable = reinterpret_cast<KingObj*>(piece);
        if (king_castlable->isMoved())
        {
          rollback();
          continue;
        }

        // Первірка чи ходила тура
        bool is_short_castle = (move.x < pos.x);  // Визначаємо коротка чи довга
        uint16_t rook_from_x = is_short_castle ? 0 : 7;
        IPiece* rook = _board[move.y][rook_from_x];

        RookObj* rook_castlable = reinterpret_cast<RookObj*>(rook);
        if (rook_castlable->isMoved())
        {
          rollback();
          continue;
        }

        // Перевіряємо кожну клітинку по шляху короля (не включаючи початкову)
        int direction = (move.x > pos.x) ? 1 : -1;
        bool path_attacked = false;

        for (int check_x = static_cast<int>(pos.x + direction); check_x != static_cast<int>(move.x + direction); check_x += direction)
        {
          if (isSquareAttacked(static_cast<uint16_t>(check_x), pos.y, piece->isWhite()))
          {
            path_attacked = true;
            break;
          }
        }

        if (path_attacked)
        {
          rollback();
          continue;
        }
      }

      // Перевірка на шах
      if (!isSquareAttacked(check_pos.x, check_pos.y, piece->isWhite()))
        legal_moves.push_back(move);

      // Відкат стану дошки
      _board[pos.y][pos.x] = piece;
      _board[move.y][move.x] = original_to;
    }

    // Конвертація для дисплея
    std::vector<Position> screen_moves;
    for (const auto& m : legal_moves)
      screen_moves.push_back(convertToScreenPos(m.x, m.y));

    return screen_moves;
  }

  Position Board::getKingPos(bool white_king) const
  {
    uint16_t king_x = 999;
    uint16_t king_y = 999;

    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        const IPiece* p = _board[y][x];
        if (p && p->getTypeID() == TYPE_KING && p->isWhite() == white_king)
        {
          king_x = x;
          king_y = y;
          break;
        }
      }
    }

    return {king_x, king_y};
  }

  bool Board::isSquareAttacked(uint16_t square_x, uint16_t square_y, bool is_white_piece) const
  {
    // Перевіряємо, чи може будь-яка ворожа фігура вдарити по цій клітинці
    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        const IPiece* enemy = _board[y][x];
        if (enemy && enemy->isWhite() != is_white_piece)
        {
          std::vector<Position> moves = enemy->getPossibleMoves(*this);
          for (const auto& m : moves)
          {
            if (m.x == square_x && m.y == square_y)
              return true;
          }
        }
      }
    }

    return false;
  }

  void Board::findAvailableMove()
  {
    const Position king_pos = getKingPos(_is_white_turn);
    bool under_attack = isSquareAttacked(king_pos.x, king_pos.y, _is_white_turn);

    // Перевіряємо всі фігури поточного гравця на наявність ходів
    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        const IPiece* p = _board[y][x];
        if (p && p->isWhite() == _is_white_turn)
        {
          Position screen_pos = convertToScreenPos(x, y);
          // Метод getPossibleMoves повертає legal_moves
          std::vector<Position> moves = getPossibleMoves(screen_pos.x, screen_pos.y);

          if (!moves.empty())
            return;  // Знайшли хоча б один можливий хід
        }
      }
    }

    if (under_attack)  // Ходів немає + шах
      _is_checkmate = true;
    else  // Ходів немає + шаху немає
      _is_stalemate = true;
  }

  void Board::checkInsufficientMaterial()
  {
    int pieces_count = 0;
    int white_bishops = 0;
    int black_bishops = 0;
    int white_bishop_color = -1;
    int black_bishop_color = -1;
    bool has_major_piece = false;

    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        const IPiece* p = _board[y][x];
        if (!p)
          continue;

        pieces_count++;
        if (p->getTypeID() == TYPE_KING)
          continue;

        if (p->getTypeID() == TYPE_BISHOP)
        {
          if (p->isWhite())
          {
            white_bishops++;
            white_bishop_color = (x + y) % 2;
          }
          else
          {
            black_bishops++;
            black_bishop_color = (x + y) % 2;
          }
        }
        else
        {
          // Якщо є хоч один кінь, пішак, тура або ферзь
          has_major_piece = true;
        }
      }
    }

    if (has_major_piece)
      return;

    // Тільки два королі
    if (pieces_count == 2)
    {
      _is_stalemate = true;
      return;
    }

    // король + слон проти короля (у будь-кого)
    if (pieces_count == 3 && (white_bishops == 1 || black_bishops == 1))
    {
      _is_stalemate = true;
      return;
    }

    // король + слон проти короля + слон (на одному кольорі)
    if (pieces_count == 4 && white_bishops == 1 && black_bishops == 1 && white_bishop_color == black_bishop_color)
      _is_stalemate = true;
  }

  const IPiece* Board::getPieceAt(uint8_t grid_x, uint8_t grid_y) const
  {
    return _board[grid_y][grid_x];
  }

  bool Board::isWhiteTurn() const
  {
    return _is_white_turn;
  }

  bool Board::isCheckmate() const
  {
    return _is_checkmate;
  }

  bool Board::isStalemate() const
  {
    return _is_stalemate;
  }

  void Board::updateObjPhysPos(IPiece* piece, uint32_t row, uint32_t col)
  {
    const uint16_t x = _X_LOC_OFFSET + (col * _SQUARE_SIZE) + (_SQUARE_SIZE - _PIECE_SIZE) / 2;
    const uint16_t y = _Y_LOC_OFFSET + (row * _SQUARE_SIZE) + (_SQUARE_SIZE - _PIECE_SIZE) / 2;
    piece->setPos(x, y);
  }

  IPiece* Board::findFreePiece(TypeID type, bool white, std::vector<bool>& used)
  {
    for (size_t i = 0; i < _pieces.size(); ++i)
    {
      if (!used[i] && _pieces[i]->getTypeID() == type && _pieces[i]->isWhite() == white)
      {
        used[i] = true;
        return _pieces[i];
      }
    }
    return nullptr;
  }

  Position Board::convertToScreenPos(uint16_t grid_x, uint16_t grid_y) const
  {
    const uint16_t screen_x = grid_x * _SQUARE_SIZE + _X_GLOB_OFFSET;
    const uint16_t screen_y = grid_y * _SQUARE_SIZE + _Y_GLOB_OFFSET;
    return {screen_x, screen_y};
  }

  Position Board::convertToGridPos(uint16_t screen_x, uint16_t screen_y) const
  {
    uint16_t grid_x = (screen_x - _X_GLOB_OFFSET) / _SQUARE_SIZE;
    uint16_t grid_y = (screen_y - _Y_GLOB_OFFSET) / _SQUARE_SIZE;

    if (grid_x > 7)
      grid_x = 7;

    if (grid_y > 7)
      grid_y = 7;

    return {grid_x, grid_y};
  }

  const Board::LastMove& Board::getLastMove() const
  {
    return _last_move;
  }

  void Board::movePiece(uint16_t x_from, uint16_t y_from, uint16_t x_to, uint16_t y_to)
  {
    const Position pos_from = convertToGridPos(x_from, y_from);
    const Position pos_to = convertToGridPos(x_to, y_to);
    x_from = pos_from.x;
    y_from = pos_from.y;
    x_to = pos_to.x;
    y_to = pos_to.y;

    const IPiece* piece = getPieceAt(pos_from.x, pos_from.y);
    if (piece == nullptr || piece->isWhite() != _is_white_turn)
      return;

    bool _has_castling{false};
    if (piece->getTypeID() == TYPE_PAWN && x_from != x_to && _board[y_to][x_to] == nullptr)  // Взяття на льоту
    {
      _board[y_from][x_to]->destroy();
      _board[y_from][x_to] = nullptr;
    }
    else if (piece->getTypeID() == TYPE_KING && __builtin_abs(static_cast<int>(x_to) - static_cast<int>(x_from)) > 1)  // Рокіровка
    {
      _has_castling = true;

      const bool is_short_castle = (x_to < x_from);  // Визначаємо коротка чи довга
      const uint16_t rook_from_x = is_short_castle ? 0 : 7;
      const uint16_t rook_to_x = is_short_castle ? 2 : 5;

      // Переставляємо короля. На дисплеї оновиться в кінці методу
      _board[y_to][x_to] = _board[y_from][x_from];
      _board[y_from][x_from] = nullptr;

      // Помітити короля, що ходив
      KingObj* king_castlable = reinterpret_cast<KingObj*>(_board[y_to][x_to]);
      king_castlable->markMoved();

      // Переставляємо туру
      IPiece* rook = _board[y_to][rook_from_x];
      _board[y_to][rook_to_x] = rook;
      _board[y_to][rook_from_x] = nullptr;

      // Помітити туру, що ходила
      RookObj* rook_castlable = reinterpret_cast<RookObj*>(rook);
      rook_castlable->markMoved();

      // Оновлюємо позицію тури на дисплеї
      updateObjPhysPos(rook, y_to, rook_to_x);
    }
    else if (_board[y_to][x_to])  // Звичайне взяття
    {
      _board[y_to][x_to]->destroy();
    }

    if (!_has_castling)
    {
      _board[y_to][x_to] = _board[y_from][x_from];
      _board[y_from][x_from] = nullptr;

      if (piece->getTypeID() == TYPE_ROOK)
      {
        RookObj* rook_castlable = reinterpret_cast<RookObj*>(_board[y_to][x_to]);
        rook_castlable->markMoved();
      }
      else if (piece->getTypeID() == TYPE_KING)
      {
        KingObj* king_castlable = reinterpret_cast<KingObj*>(_board[y_to][x_to]);
        king_castlable->markMoved();
      }
    }

    // Перетворення
    if (piece->getTypeID() == TYPE_PAWN && (y_to == 0 || y_to == 7))
      static_cast<PawnObj*>(_board[y_to][x_to])->turnIntoQueen();

    _last_move.from_x = x_from;
    _last_move.from_y = y_from;
    _last_move.to_x = x_to;
    _last_move.to_y = y_to;
    _last_move.piece_type = piece->getTypeID();

    updateObjPhysPos(_board[y_to][x_to], y_to, x_to);

    _is_white_turn = !_is_white_turn;

    findAvailableMove();
    if (!_is_checkmate)
      checkInsufficientMaterial();

    int16_t rot_angle = 180;
    if (_is_white_turn)
      rot_angle = 0;

    for (const auto& p : _pieces)
      p->rotateSprite(rot_angle);

    // dumpBoard();
  }

  void Board::dumpBoard()
  {
    // for (int row = 6; row < 8; row++)
    // {
    //   std::cout << "row " << row << ": ";
    //   for (int col = 0; col < 8; col++)
    //   {
    //     IPiece* p = _board[row][col];
    //     if (p == nullptr)
    //       std::cout << "[ ] ";
    //     else
    //       std::cout << "[" << p->getTypeID() << "] ";
    //   }
    //   std::cout << "\n";
    // }
    // std::cout << "\n";
  }

}  // namespace chess
