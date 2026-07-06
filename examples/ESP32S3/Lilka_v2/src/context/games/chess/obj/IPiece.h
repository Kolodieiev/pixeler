#pragma once

#include <vector>

#include "Board.h"
#include "IMoveStrategy.h"
#include "game/IGameObject.h"

using namespace pixeler;

namespace chess
{
  class IPiece : public IGameObject
  {
  public:
    IPiece(uint32_t id, IGameScene& game_scene, SfxPlayer& audio, uint16_t type_id, const unsigned short* const sprite_arr[], IMoveStrategy* movement);
    virtual ~IPiece() = 0;
    std::vector<Position> getPossibleMoves(const Board& board) const;

    virtual void __update() override;
    virtual void serialize(DataStream& ds) const override;
    virtual void deserialize(DataStream& ds) override;
    virtual size_t getDataSize() const override;

    void setIsWhite(bool state);
    bool isWhite() const;
    void destroy();

    void rotateSprite(int16_t angle);

  protected:
    // Потрібно лишити protected щоб пішак міг змінити поля
    //
    const unsigned short* const* _SPRITE_ARR{nullptr};
    IMoveStrategy* _movement{nullptr};
    //

  private:
    bool _is_white{true};
  };
}  // namespace chess
