#pragma once
#include "bitboard.hpp"
#include "square.hpp"
#include <array>
#include <cstddef>

namespace Magic {
namespace detail {

inline constexpr std::size_t ROOK_TABLE_SIZE = 102400;
inline constexpr std::size_t BISHOP_TABLE_SIZE = 5248;

struct Entry {
    BitBoard mask = 0;
    BitBoard magic = 0;
    int shift = 0;
    int offset = 0;
};

} // namespace detail

struct MagicKeys {
    std::array<detail::Entry, 64> rook;
    std::array<detail::Entry, 64> bishop;
    std::array<BitBoard, detail::ROOK_TABLE_SIZE> rook_table;
    std::array<BitBoard, detail::BISHOP_TABLE_SIZE> bishop_table;

    MagicKeys();
};

extern const MagicKeys keys;

inline BitBoard rook_attacks(Square sq, BitBoard occupancy) {
    const auto &e = keys.rook[sq.raw()];
    int index = ((occupancy & e.mask).raw() * e.magic.raw()) >> e.shift;
    return BitBoard(keys.rook_table[e.offset + index]);
}

inline BitBoard bishop_attacks(Square sq, BitBoard occupancy) {
    const auto &e = keys.bishop[sq.raw()];
    int index = ((occupancy & e.mask).raw() * e.magic.raw()) >> e.shift;
    return BitBoard(keys.bishop_table[e.offset + index]);
}

extern const std::array<std::array<BitBoard, 64>, 64> RAY_BETWEEN;

} // namespace Magic
