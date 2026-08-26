#include "ChessOfflineContext.h"

#include "ChessClientContext.h"
#include "ChessContext.h"
#include "ChessServerContext.h"
#include "scene/ChessScene.h"

namespace chess
{
  ChessOfflineContext::ChessOfflineContext(uint8_t player_num)
  {
    if (player_num == 1)  // Проти ПК // TODO
    {
      _scene = new ChessScene(_stored_objs, 1);
    }
    else
    {
      _scene = new ChessScene(_stored_objs, 2);
    }
  }

  ChessOfflineContext::~ChessOfflineContext()
  {
  }

  bool ChessOfflineContext::loop()
  {
    return true;
  }

  void ChessOfflineContext::update()
  {
    _scene->update();

    if (!_scene->isFinished() && !_scene->isReleased())
    {
      _scene->update();
    }
    else
    {
      delete _scene;
      openContext(new ChessContext());
    }
  }
}  // namespace chess
