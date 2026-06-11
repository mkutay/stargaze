#pragma once
#include "enums.hpp"
#include <cassert>

/**
 * Move encoding (16 bits):
 *
 * `ffffffttttttssss`
 *
 * where f = from square (0-63), t = to square (0-63), s = special flags.
 *
 * The from and to values represent the source and destination squares of the
 * move, encoded as indices of the board.
 *
 * Each bit of the special flags is used as:
 *
 * 1) promotion
 * 2) capture
 * 3) special 1
 * 4) special 2
 */
class Move {
    uint16_t m_move;

  public:
    const static uint8_t QUIET = 0b0000;
    const static uint8_t DOUBLE_PAWN_PUSH = 0b0001;
    const static uint8_t KING_SIDE_CASTLE = 0b0010;
    const static uint8_t QUEEN_SIDE_CASTLE = 0b0011;
    const static uint8_t CAPTURE = 0b0100;
    const static uint8_t EN_PASSANT = 0b0101;
    const static uint8_t KNIGHT_PROMOTION = 0b1000;
    const static uint8_t BISHOP_PROMOTION = 0b1001;
    const static uint8_t ROOK_PROMOTION = 0b1010;
    const static uint8_t QUEEN_PROMOTION = 0b1011;
    const static uint8_t KNIGHT_PROMOTION_CAPTURE = 0b1100;
    const static uint8_t BISHOP_PROMOTION_CAPTURE = 0b1101;
    const static uint8_t ROOK_PROMOTION_CAPTURE = 0b1110;
    const static uint8_t QUEEN_PROMOTION_CAPTURE = 0b1111;

    Move() = delete;
    Move(uint16_t move) : m_move(move) {}
    Move(int from, int to, int flags)
        : m_move(from | (to << 6) | (flags << 12)) {}
    int from() const { return m_move & 0x3f; }
    int to() const { return (m_move >> 6) & 0x3f; }
    int flags() const { return (m_move >> 12) & 0x0f; }
    bool is_promotion() const { return (m_move >> 15) & 0x01; }
    bool is_capture() const { return (m_move >> 14) & 0x01; }
    bool is_en_passant() const { return flags() == 0b0101; }
    bool is_castle() const { return flags() == 0b0010 || flags() == 0b0011; }

    /**
     * Return the piece type that a pawn is promoted to in this move. Only valid
     * if this move is a promotion. The colour is inferred from the board turn
     * at the time the move is applied.
     */
    Piece promotion_piece() const {
        assert(is_promotion());

        int specials = flags() & 0b0011;
        if (specials == 0)
            return Piece::KNIGHT;
        if (specials == 1)
            return Piece::BISHOP;
        if (specials == 2)
            return Piece::ROOK;
        return Piece::QUEEN;
    }
    bool operator==(const Move &other) const { return m_move == other.m_move; }
};
