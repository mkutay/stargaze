#pragma once
#include "enums.hpp"
#include "square.hpp"
#include <cassert>

/**
 * Move encoding (16 bits):
 *
 * `ssssttttttffffff`
 *
 * where s = special flags (bits 12-15), t = to square (bits 6-11), f = from
 * square (bits 0-5).
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
    constexpr const static uint8_t QUIET = 0b0000;
    constexpr const static uint8_t DOUBLE_PAWN_PUSH = 0b0001;
    constexpr const static uint8_t KING_SIDE_CASTLE = 0b0010;
    constexpr const static uint8_t QUEEN_SIDE_CASTLE = 0b0011;
    constexpr const static uint8_t CAPTURE = 0b0100;
    constexpr const static uint8_t EN_PASSANT = 0b0101;
    constexpr const static uint8_t KNIGHT_PROMOTION = 0b1000;
    constexpr const static uint8_t BISHOP_PROMOTION = 0b1001;
    constexpr const static uint8_t ROOK_PROMOTION = 0b1010;
    constexpr const static uint8_t QUEEN_PROMOTION = 0b1011;
    constexpr const static uint8_t KNIGHT_PROMOTION_CAPTURE = 0b1100;
    constexpr const static uint8_t BISHOP_PROMOTION_CAPTURE = 0b1101;
    constexpr const static uint8_t ROOK_PROMOTION_CAPTURE = 0b1110;
    constexpr const static uint8_t QUEEN_PROMOTION_CAPTURE = 0b1111;

    constexpr static uint8_t create_flags(bool is_capture) {
        return is_capture << 2;
    }

    constexpr Move() = delete;
    constexpr Move(uint16_t move) : m_move(move) {}
    constexpr Move(Square from, Square to, int flags)
        : m_move(from | (to << 6) | (flags << 12)) {}
    constexpr Square from() const { return m_move & 0x3f; }
    constexpr Square to() const { return (m_move >> 6) & 0x3f; }
    constexpr uint8_t flags() const { return (m_move >> 12) & 0x0f; }
    constexpr bool is_promotion() const { return (m_move >> 15) & 0x01; }
    constexpr bool is_capture() const { return (m_move >> 14) & 0x01; }
    constexpr bool is_en_passant() const { return flags() == EN_PASSANT; }
    constexpr bool is_double_pawn_push() const {
        return flags() == DOUBLE_PAWN_PUSH;
    }
    constexpr bool is_castle() const {
        return flags() == KING_SIDE_CASTLE || flags() == QUEEN_SIDE_CASTLE;
    }

    /**
     * Return the piece type that a pawn is promoted to in this move. Only valid
     * if this move is a promotion. The colour is inferred from the board turn
     * at the time the move is applied.
     */
    constexpr Piece promotion_piece() const {
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

    constexpr bool operator==(const Move &other) const {
        return m_move == other.m_move;
    }
};

static_assert(sizeof(Move) == sizeof(uint16_t));
