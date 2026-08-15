#include "IMoveStrategy.h"

#include "IPiece.h"

namespace chess
{
  std::vector<Position> IMoveStrategy::calcSlidingMoves(uint8_t x_pos, uint8_t y_pos, const Board& board, std::initializer_list<std::pair<int, int>> directions) const
  {
    std::vector<Position> moves;

    // Конвертуємо вхідні координати (0-7)
    Position grid_pos = board.convertToGridPos(x_pos, y_pos);

    // Отримуємо фігуру, щоб знати її колір
    const IPiece* current_piece = board.getPieceAt(grid_pos.x, grid_pos.y);
    if (!current_piece)
      return moves;

    const bool is_white = current_piece->isWhite();

    // Проходимо по кожному напрямку з вхідного параметра directions
    for (auto& dir : directions)
    {
      for (int i = 1; i < 8; ++i)
      {
        int next_x = static_cast<int>(grid_pos.x) + dir.first * i;
        int next_y = static_cast<int>(grid_pos.y) + dir.second * i;

        // Перевірка меж дошки
        if (next_x < 0 || next_x >= 8 || next_y < 0 || next_y >= 8)
          break;

        const IPiece* target = board.getPieceAt(next_x, next_y);

        if (target == nullptr)  // Клітинка порожня
        {
          moves.push_back({static_cast<uint16_t>(next_x), static_cast<uint16_t>(next_y)});
        }
        else  // Шлях перекрито (своєю або ворожою фігурою)
        {
          if (target->isWhite() != is_white)  // Якщо фігура ворожа — додаємо взяття
            moves.push_back({static_cast<uint16_t>(next_x), static_cast<uint16_t>(next_y)});

          break;
        }
      }
    }

    return moves;
  }
}  // namespace chess
