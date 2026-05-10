#include "QueenMove.h"

#include "../IPiece.h"

namespace chess
{
  std::vector<Position> QueenMove::calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const
  {
    return calcSlidingMoves(x_pos, y_pos, board, {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {0, 1}, {0, -1}, {1, 0}, {-1, 0}});
  }
}  // namespace chess
