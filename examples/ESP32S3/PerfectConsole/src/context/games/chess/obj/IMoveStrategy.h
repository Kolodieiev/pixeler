#pragma once
#include <initializer_list>
#include <vector>

#include "Board.h"

namespace chess
{
  class IMoveStrategy
  {
  public:
    IMoveStrategy() {}
    virtual ~IMoveStrategy() {}
    virtual std::vector<Position> calcMoves(uint8_t x_pos, uint8_t y_pos, const Board& board) const = 0;

    IMoveStrategy(const IMoveStrategy&) = delete;
    IMoveStrategy& operator=(const IMoveStrategy&) = delete;
    IMoveStrategy& operator=(IMoveStrategy&& other) = delete;

  protected:
    std::vector<Position> calcSlidingMoves(uint8_t x_pos, uint8_t y_pos, const Board& board, std::initializer_list<std::pair<int, int>> directions) const;

  private:
  };
}  // namespace chess
