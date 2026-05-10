#include "PawnMove.h"

#include "../IPiece.h"

namespace chess
{
  std::vector<Position> PawnMove::calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const
  {
    std::vector<Position> moves;

    // Конвертуємо глобальні координати фігури в шахову сітку
    const Position pos = board.convertToGridPos(x_pos, y_pos);
    x_pos = pos.x;
    y_pos = pos.y;

    // Визначаємо поточну фігуру, щоб отримати колір
    const IPiece* current_piece = board.getPieceAt(x_pos, y_pos);
    if (!current_piece)
      return moves;

    const bool is_white = current_piece->isWhite();

    // Напрямок руху: білі зазвичай йдуть вгору (зменшення індексу row), чорні — вниз
    const int direction = is_white ? -1 : 1;
    const int start_row = is_white ? 6 : 1;

    // 1. Хід вперед на одну клітинку
    const int next_y = y_pos + direction;
    if (next_y >= 0 && next_y < 8)
    {
      if (board.getPieceAt(x_pos, next_y) == nullptr)
      {
        moves.push_back({static_cast<uint16_t>(x_pos), static_cast<uint16_t>(next_y)});

        // 2. Хід вперед на дві клітинки (тільки якщо перша була порожня)
        int double_next_y = y_pos + (2 * direction);
        if (y_pos == start_row && board.getPieceAt(x_pos, double_next_y) == nullptr)
          moves.push_back({static_cast<uint16_t>(x_pos), static_cast<uint16_t>(double_next_y)});
      }
    }

    // 3. Взяття ворожих фігур по діагоналі
    const int diag_offsets[] = {-1, 1};  // вліво і вправо
    for (int dx : diag_offsets)
    {
      const int target_x = x_pos + dx;
      const int target_y = y_pos + direction;

      if (target_x >= 0 && target_x < 8 && target_y >= 0 && target_y < 8)
      {
        const IPiece* target_piece = board.getPieceAt(target_x, target_y);
        if (target_piece != nullptr && target_piece->isWhite() != is_white)
          moves.push_back({static_cast<uint16_t>(target_x), static_cast<uint16_t>(target_y)});
      }
    }

    // 4. Взяття на проході (En Passant)
    // Перевіряємо тільки якщо пішак на потрібній горизонталі (3 для білих, 4 для чорних у 0-7 індексах)
    const int en_passant_row = is_white ? 3 : 4;
    if (y_pos == en_passant_row)
    {
      const Board::LastMove& last = board.getLastMove();

      // Якщо останній хід був зроблений ворожим пішаком на 2 клітинки
      if (last.piece_type == TYPE_PAWN && __builtin_abs(last.from_y - last.to_y) == 2)
      {
        // І цей пішак стоїть ліворуч або праворуч від нашого
        if (last.to_y == y_pos && __builtin_abs(last.to_x - x_pos) == 1)
        {
          // Додаємо хід по діагоналі "за спину" ворогу
          moves.push_back({static_cast<uint16_t>(last.to_x), static_cast<uint16_t>(y_pos + direction)});
        }
      }
    }

    return moves;
  }
}  // namespace chess
