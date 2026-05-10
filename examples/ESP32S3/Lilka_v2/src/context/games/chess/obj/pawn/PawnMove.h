#pragma once
#include "../IMoveStrategy.h"

namespace chess
{
  class PawnMove : public IMoveStrategy
  {
  public:
    PawnMove() {}
    virtual ~PawnMove() {}

    virtual std::vector<Position> calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const override;
  };
}  // namespace chess
