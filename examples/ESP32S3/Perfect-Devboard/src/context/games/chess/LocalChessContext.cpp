#include "LocalChessContext.h"

#include "ChessClientContext.h"
#include "ChessContext.h"
#include "ChessServerContext.h"
#include "scene/AIChessScene.h"
#include "scene/SingleChessScene.h"

namespace chess
{
  LocalChessContext::LocalChessContext(uint8_t player_num)
  {
    if (player_num == 1)  // Проти ПК // TODO
    {
      _scene = new SingleChessScene(_stored_objs);
    }
    else
    {
      _scene = new SingleChessScene(_stored_objs);
    }
  }

  LocalChessContext::~LocalChessContext()
  {
  }

  bool LocalChessContext::loop()
  {
    return true;
  }

  void LocalChessContext::update()
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
