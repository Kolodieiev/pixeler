#pragma once

#include <cstdint>

namespace chess
{
  class ICastlable
  {
  public:
    ICastlable();
    virtual ~ICastlable() = 0;
    void markMoved();
    bool isMoved() const;

  private:
    bool _is_moved{false};
  };
}  // namespace chess
