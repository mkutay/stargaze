#pragma once
#include "enums.hpp"
#include <cassert>
#include <utility>

class Move {
  public:
    uint16_t m_move; // 16 bits (6 for from and to, 4 for type)
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
    Piece promotion_piece(Piece colour) const {
#ifdef DEBUG
        assert(is_promotion());
#endif
        static_assert(std::to_underlying(Piece::WHITE) == 13);
        static_assert(std::to_underlying(Piece::BLACK) == 14);

        auto c = std::to_underlying(colour);
        assert(c >= 13);
        c -= 13; // 0 for white, 1 for black

        int specials = flags() & 0b0011;
        if (specials == 0)
            return static_cast<Piece>(c * 6 + 1); // knight
        if (specials == 1)
            return static_cast<Piece>(c * 6 + 2); // bishop
        if (specials == 2)
            return static_cast<Piece>(c * 6 + 3); // rook
        return static_cast<Piece>(c * 6 + 4);     // queen
    }
    bool operator==(const Move &other) const { return m_move == other.m_move; }
};
