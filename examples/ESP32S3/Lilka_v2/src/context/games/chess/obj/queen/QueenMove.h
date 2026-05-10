#pragma once
#include "../IMoveStrategy.h"

namespace chess
{
  class QueenMove : public IMoveStrategy
  {
  public:
    QueenMove() {}
    virtual ~QueenMove() {}

    virtual std::vector<Position> calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const override;
  };
}  // namespace chess
