#include "ICastlable.h"

namespace chess
{
  ICastlable::ICastlable()
  {
  }

  ICastlable::~ICastlable()
  {
  }

  void ICastlable::markMoved()
  {
    _is_moved = true;
  }

  bool ICastlable::isMoved() const
  {
    return _is_moved;
  }
}  // namespace chess
