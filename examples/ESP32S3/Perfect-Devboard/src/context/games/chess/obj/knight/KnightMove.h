#pragma once
#include "../IMoveStrategy.h"

namespace chess
{
  class KnightMove : public IMoveStrategy
  {
  public:
    KnightMove() {}
    virtual ~KnightMove() {}

    virtual std::vector<Position> calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const override;
  };
}  // namespace chess
