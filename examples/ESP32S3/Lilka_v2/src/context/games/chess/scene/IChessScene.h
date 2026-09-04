#pragma once

#include "../obj/Board.h"
#include "../obj/player/CameraObj.h"
#include "game/2D/IGameScene2D.h"
#include "widget/text/Label.h"

namespace chess
{
  class IChessScene : public IGameScene2D
  {
  public:
    explicit IChessScene(DataStream& stored_objs, bool is_white);
    virtual ~IChessScene();

    virtual void update() override;

  protected:
    virtual void onTriggered(uint16_t id) = 0;

    void buildTerrain();      // Завантажити ігровий рівень
    void createMainObj();     // Створюємо об'єкт прив'язки камери
    void prepareBoard();      // Розмістити шахи на стартові позиції
    void createSpiteTmpls();  // Згенерувати спрайтові шаблони для ігрових об'єктів

    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    void handleOkClick();
    void clearCurrSelect();

  protected:
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
    const bool IS_WHITE; // TODO sort to private
  };
}  // namespace chess
