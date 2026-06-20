#pragma once
#include "piece.hpp"
#include "square.hpp"
#include <cassert>
#include <string>

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

    constexpr const static std::array PROMOTION_PIECES = {
        KNIGHT_PROMOTION, BISHOP_PROMOTION, ROOK_PROMOTION, QUEEN_PROMOTION};
    constexpr const static std::array PROMOTION_CAPTURE_PIECES = {
        KNIGHT_PROMOTION_CAPTURE, BISHOP_PROMOTION_CAPTURE,
        ROOK_PROMOTION_CAPTURE, QUEEN_PROMOTION_CAPTURE};

    constexpr static uint8_t create_flags(bool is_capture) {
        return is_capture << 2;
    }

    constexpr Move() : m_move(0) {}
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
    constexpr bool is_quiet() const { return (flags() & 0b1100) == 0; }

    /**
     * Return the piece type that a pawn is promoted to in this move. Only valid
     * if this move is a promotion. The colour is inferred from the board turn
     * at the time the move is applied.
     */
    constexpr Piece promotion_piece() const {
        assert(is_promotion());

        int specials = flags() & 0b0011;
        if (specials == 0)
            return PP::KNIGHT;
        if (specials == 1)
            return PP::BISHOP;
        if (specials == 2)
            return PP::ROOK;
        return PP::QUEEN;
    }

    constexpr bool operator==(const Move &other) const {
        return m_move == other.m_move;
    }

    constexpr std::string to_string() const {
        std::string ret = from().to_string() + to().to_string();
        if (is_promotion()) {
            ret += promotion_piece().to_string();
        }
        return ret;
    }
};

static_assert(sizeof(Move) == sizeof(uint16_t));

static_assert(Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).from() == SQ::E2);
static_assert(Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).to() == SQ::E4);
static_assert(Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).flags() ==
              Move::DOUBLE_PAWN_PUSH);
static_assert(
    Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).is_double_pawn_push());
static_assert(!Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).is_capture());
static_assert(!Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).is_promotion());
static_assert(Move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH).to_string() ==
              "e2e4");

static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).from() ==
              SQ::D7);
static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).to() ==
              SQ::C8);
static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).flags() ==
              Move::QUEEN_PROMOTION_CAPTURE);
static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).is_capture());
static_assert(
    Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).is_promotion());
static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE)
                  .promotion_piece() == PP::QUEEN);
static_assert(Move(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE).to_string() ==
              "d7c8q");

static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).from() == SQ::E5);
static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).to() == SQ::F6);
static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).flags() ==
              Move::EN_PASSANT);
static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).is_capture());
static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).is_en_passant());
static_assert(Move(SQ::E5, SQ::F6, Move::EN_PASSANT).to_string() == "e5f6");
