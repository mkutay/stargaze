#pragma once
#include <string>
#include <cassert>
#include "enums.hpp"

class Move {
public:
    u_int16_t m_move; // 16 bits (6 for from and to, 4 for type)
    Move() : m_move(0) {}
    Move(u_int16_t move) : m_move(move) {}
    Move(int from, int to, int flags) : m_move(from | (to << 6) | (flags << 12)) {}
    int from() const { return m_move & 0x3f; }
    int to() const { return (m_move >> 6) & 0x3f; }
    int flags() const { return (m_move >> 12) & 0x0f; }
    bool is_promotion() const { return (m_move >> 15) & 0x01; }
    bool is_capture() const { return (m_move >> 14) & 0x01; }
    bool is_en_passant() const { return flags() == 0b0101; }
    bool is_castle() const { return flags() == 0b0010 || flags() == 0b0011; }
    Piece promotion_piece() const {
#ifdef DEBUG
        assert(is_promotion());
#endif
        int specials = flags() & 0b0011;
        if (specials == 0) return KNIGHT; // knight
        if (specials == 1) return BISHOP; // bishop
        if (specials == 2) return ROOK; // rook
        return QUEEN; // queen
    }
    bool operator==(const Move& other) const {
        return m_move == other.m_move;
    }
};