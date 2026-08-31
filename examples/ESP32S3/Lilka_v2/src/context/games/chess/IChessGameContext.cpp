#include "IChessGameContext.h"

namespace chess
{
  IChessGameContext::IChessGameContext()
  {
    setCpuFrequency(FREQ_MAX);
  }

  IChessGameContext::~IChessGameContext()
  {

  }
}  // namespace chess
