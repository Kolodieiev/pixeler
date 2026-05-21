#pragma once

#include "../obj/Board.h"
#include "../obj/player/CameraObj.h"
#include "game/IGameScene.h"
#include "widget/text/Label.h"

using namespace pixeler;

namespace chess
{
  class ChessScene : public IGameScene
  {
  public:
    explicit ChessScene(DataStream& stored_objs, uint8_t players_num = 1);
    virtual ~ChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) override;

  private:
    enum GameMode : uint8_t
    {
      GAME_MODE_ONE_PL = 1,
      GAME_MODE_TWO_PL,
      GAME_MODE_CLIENT,
      GAME_MODE_SERVER,
    };

    void buildTerrain();   // Завантажити ігровий рівень
    void createMainObj();  // Створюємо об'єкт прив'язки камери
    void prepareBoard();   // Розмістити шахи на стартові позиції
    //
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    void handleOkClick();
    void clearCurrSelect();

  private:
    std::vector<Position> _possible_moves;
    Board _board;
    CameraObj* _camera{nullptr};
    Label* _msg_lbl{nullptr};

    uint16_t _cur_x{0};
    uint16_t _cur_y{0};
    uint16_t _cur_x_selected{0};
    uint16_t _cur_y_selected{0};

    uint8_t _cur_poss_position{0};

    bool _is_piece_selected{false};

    const GameMode _GAME_MODE;
  };
}  // namespace chess
