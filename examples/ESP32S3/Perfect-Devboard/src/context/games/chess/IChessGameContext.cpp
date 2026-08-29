#include "IChessGameContext.h"

namespace chess
{
  IChessGameContext::IChessGameContext()
  {
    setCpuFrequency(FREQ_MAX);
  }

  IChessGameContext::~IChessGameContext()
  {
    if (_scene)
      delete (_scene);
  }
}  // namespace chess
