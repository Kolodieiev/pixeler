#pragma once
#include <vector>

#include "TypeID.h"

namespace chess
{
  class IPiece;

  struct Position
  {
    uint16_t x{0};
    uint16_t y{0};
  };

  class Board
  {
  public:
    struct LastMove
    {
      uint16_t from_x{999};
      uint16_t from_y{999};
      uint16_t to_x{999};
      uint16_t to_y{999};
      uint16_t piece_type{TYPE_NONE};
    };

    explicit Board(uint16_t x_glob_offset, uint16_t y_glob_offset, uint16_t x_loc_offset, uint16_t y_loc_offset, uint16_t square_size, uint16_t piece_size);
    ~Board();

    void addPiece(IPiece* piece);
    void reset();
    bool isWhiteTurn() const;
    bool isCheckmate() const;
    bool isStalemate() const;

    std::vector<Position> getPossibleMoves(uint16_t x, uint16_t y) const;

    const IPiece* getPieceAt(uint8_t grid_x, uint8_t grid_y) const;

    void movePiece(uint16_t x_from, uint16_t y_from, uint16_t x_to, uint16_t y_to);

    Position convertToGridPos(uint16_t screen_x, uint16_t screen_y) const;

    const LastMove& getLastMove() const;

  private:
    void updateObjPhysPos(IPiece* piece, uint32_t row, uint32_t col);
    IPiece* findFreePiece(TypeID type, bool white, std::vector<bool>& used);
    Position convertToScreenPos(uint16_t grid_x, uint16_t grid_y) const;
    Position getKingPos(bool white_king) const;
    bool isSquareAttacked(uint16_t square_x, uint16_t square_y, bool is_white_piece) const;
    void findAvailableMove();
    void checkInsufficientMaterial();

    void dumpBoard();

  private:
    std::vector<IPiece*> _pieces;
    LastMove _last_move;
    mutable IPiece* _board[8][8];
    const uint16_t _X_GLOB_OFFSET;
    const uint16_t _Y_GLOB_OFFSET;
    const uint16_t _X_LOC_OFFSET;
    const uint16_t _Y_LOC_OFFSET;
    const uint16_t _SQUARE_SIZE;
    const uint16_t _PIECE_SIZE;

    bool _is_white_turn{true};
    bool _is_checkmate{false};
    bool _is_stalemate{false};
  };
}  // namespace chess
