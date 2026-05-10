#include "KingMove.h"

#include "../IPiece.h"

namespace chess
{
  std::vector<Position> KingMove::calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const
  {
    std::vector<Position> moves;

    // 1. Конвертуємо в координати сітки (0-7)
    const Position grid_pos = board.convertToGridPos(x_pos, y_pos);
    const IPiece* self = board.getPieceAt(grid_pos.x, grid_pos.y);

    if (!self)
      return moves;
    const bool is_white = self->isWhite();

    // 2. Стандартні ходи на 1 клітинку
    const int offsets[8][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (auto& off : offsets)
    {
      const int nx = static_cast<int>(grid_pos.x) + off[0];
      const int ny = static_cast<int>(grid_pos.y) + off[1];

      if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
      {
        const IPiece* target = board.getPieceAt(nx, ny);
        // Додаємо, якщо порожньо або ворог
        if (target == nullptr || target->isWhite() != is_white)
          moves.push_back({static_cast<uint16_t>(nx), static_cast<uint16_t>(ny)});
      }
    }

    const int start_row = is_white ? 7 : 0;

    const IPiece* rook_short = board.getPieceAt(0, start_row);
    if (rook_short && rook_short->getTypeID() == TYPE_ROOK)
    {
      if (!board.getPieceAt(1, start_row) && !board.getPieceAt(2, start_row))
        moves.push_back({1, static_cast<uint16_t>(start_row)});
    }

    const IPiece* rook_long = board.getPieceAt(7, start_row);
    if (rook_long && rook_long->getTypeID() == TYPE_ROOK)
    {
      if (!board.getPieceAt(6, start_row) && !board.getPieceAt(5, start_row) && !board.getPieceAt(4, start_row))
        moves.push_back({6, static_cast<uint16_t>(start_row)});
    }

    return moves;
  }
}  // namespace chess
