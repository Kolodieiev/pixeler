#include "KnightMove.h"

#include "../IPiece.h"

namespace chess
{
  std::vector<Position> KnightMove::calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const
  {
    std::vector<Position> moves;

    // Конвертуємо вхідні координати в сітку 0-7
    Position grid_pos = board.convertToGridPos(x_pos, y_pos);

    const IPiece* self = board.getPieceAt(grid_pos.x, grid_pos.y);
    if (!self)
      return moves;

    const bool is_white = self->isWhite();

    // Всі 8 можливих стрибків коня (літера "Г")
    const int offsets[8][2] = {{1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}};

    for (auto& off : offsets)
    {
      int nx = static_cast<int>(grid_pos.x) + off[0];
      int ny = static_cast<int>(grid_pos.y) + off[1];

      // Перевірка меж дошки
      if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
      {
        const IPiece* target = board.getPieceAt(nx, ny);

        // Кінь може стати на клітинку, якщо вона порожня АБО там ворог
        if (target == nullptr || target->isWhite() != is_white)
          moves.push_back({static_cast<uint16_t>(nx), static_cast<uint16_t>(ny)});
      }
    }

    return moves;
  }
}  // namespace chess
