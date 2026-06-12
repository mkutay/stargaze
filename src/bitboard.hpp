#pragma once
#include "square.hpp"
#include <cassert>
#include <string>

class BitBoard {
  private:
    uint64_t bb;

  public:
    /**
     * The default constructor is deleted to prevent accidentally creating an
     * uninitialised bitboard, preserving the variant.
     */
    constexpr BitBoard() = delete;
    constexpr BitBoard(uint64_t _bb) : bb(_bb) {}
    constexpr BitBoard(Square sq) : bb(1ull << sq) {
        assert(sq >= 0 && sq < 64);
    }

    constexpr operator uint64_t() const { return bb; }

    constexpr bool has_square(Square sq) const { return (bb >> sq) & 1; }

    /**
     * Get the index of the most significant bit (MSB).
     */
    constexpr Square get_msb_square() const { return 63 - __builtin_clzll(bb); }

    /**
     * Get the index of the least significant bit (LSB). Same as using the De
     * Brunj algorithm to find the index of the LSB, but faster!
     */
    constexpr Square get_lsb_square() const { return __builtin_ctzll(bb); }

    /**
     * Get the bitboard with only the least significant bit (LSB) set. This is
     * useful for iterating through the bits of a bb, as it allows you to
     * isolate and remove the LSB in each iteration.
     */
    constexpr BitBoard lsb() const { return bb & -bb; }

    /**
     * Get the bitboard with only the most significant bit (MSB) set.
     */
    constexpr BitBoard msb() const { return 1ull << get_msb_square(); }

    /**
     * Get the index of the least significant bit (LSB) and remove it from the
     * bitboard, returning the index as a Square.
     */
    Square get_square_pop() {
        const Square sq = get_lsb_square();
        bb &= bb - 1;
        return sq;
    }

    constexpr int count() const { return __builtin_popcountll(bb); }

    constexpr BitBoard operator|(const BitBoard &o) const { return bb | o.bb; }
    constexpr BitBoard operator&(const BitBoard &o) const { return bb & o.bb; }
    constexpr BitBoard operator^(const BitBoard &o) const { return bb ^ o.bb; }
    constexpr BitBoard operator|(const uint64_t &o) const { return bb | o; }
    constexpr BitBoard operator&(const uint64_t &o) const { return bb & o; }
    constexpr BitBoard operator^(const uint64_t &o) const { return bb ^ o; }
    constexpr BitBoard operator<<(const int8_t s) const { return bb << s; }
    constexpr BitBoard operator>>(const int8_t s) const { return bb >> s; }
    constexpr BitBoard operator~() const { return ~bb; }

    BitBoard &operator|=(const BitBoard &o) {
        bb |= o.bb;
        return *this;
    }
    BitBoard &operator&=(const BitBoard &o) {
        bb &= o.bb;
        return *this;
    }
    BitBoard &operator^=(const BitBoard &o) {
        bb ^= o.bb;
        return *this;
    }
    BitBoard &operator|=(const uint64_t &o) {
        bb |= o;
        return *this;
    }
    BitBoard &operator&=(const uint64_t &o) {
        bb &= o;
        return *this;
    }
    BitBoard &operator^=(const uint64_t &o) {
        bb ^= o;
        return *this;
    }

    constexpr bool operator!=(const BitBoard &o) const { return bb != o.bb; }
    constexpr bool operator!=(const uint64_t &o) const { return bb != o; }
    constexpr bool operator==(const BitBoard &o) const { return bb == o.bb; }
    constexpr bool operator==(const uint64_t &o) const { return bb == o; }

    void set_bit(Square sq) { bb |= (1ull << sq); }
    void erase_bit(Square sq) { bb &= ~(1ull << sq); }
    void toggle_bit(Square sq) { bb ^= (1ull << sq); }

    std::string to_string() {
        BitBoard copy = bb;
        std::string board_str;
        while (copy) {
            board_str += copy.get_square_pop().to_string() + " ";
        }
        return board_str;
    }
};

namespace BB {
constexpr BitBoard A1 = SQ::A1;
constexpr BitBoard B1 = SQ::B1;
constexpr BitBoard C1 = SQ::C1;
constexpr BitBoard D1 = SQ::D1;
constexpr BitBoard E1 = SQ::E1;
constexpr BitBoard F1 = SQ::F1;
constexpr BitBoard G1 = SQ::G1;
constexpr BitBoard H1 = SQ::H1;
constexpr BitBoard A2 = SQ::A2;
constexpr BitBoard B2 = SQ::B2;
constexpr BitBoard C2 = SQ::C2;
constexpr BitBoard D2 = SQ::D2;
constexpr BitBoard E2 = SQ::E2;
constexpr BitBoard F2 = SQ::F2;
constexpr BitBoard G2 = SQ::G2;
constexpr BitBoard H2 = SQ::H2;
constexpr BitBoard A3 = SQ::A3;
constexpr BitBoard B3 = SQ::B3;
constexpr BitBoard C3 = SQ::C3;
constexpr BitBoard D3 = SQ::D3;
constexpr BitBoard E3 = SQ::E3;
constexpr BitBoard F3 = SQ::F3;
constexpr BitBoard G3 = SQ::G3;
constexpr BitBoard H3 = SQ::H3;
constexpr BitBoard A4 = SQ::A4;
constexpr BitBoard B4 = SQ::B4;
constexpr BitBoard C4 = SQ::C4;
constexpr BitBoard D4 = SQ::D4;
constexpr BitBoard E4 = SQ::E4;
constexpr BitBoard F4 = SQ::F4;
constexpr BitBoard G4 = SQ::G4;
constexpr BitBoard H4 = SQ::H4;
constexpr BitBoard A5 = SQ::A5;
constexpr BitBoard B5 = SQ::B5;
constexpr BitBoard C5 = SQ::C5;
constexpr BitBoard D5 = SQ::D5;
constexpr BitBoard E5 = SQ::E5;
constexpr BitBoard F5 = SQ::F5;
constexpr BitBoard G5 = SQ::G5;
constexpr BitBoard H5 = SQ::H5;
constexpr BitBoard A6 = SQ::A6;
constexpr BitBoard B6 = SQ::B6;
constexpr BitBoard C6 = SQ::C6;
constexpr BitBoard D6 = SQ::D6;
constexpr BitBoard E6 = SQ::E6;
constexpr BitBoard F6 = SQ::F6;
constexpr BitBoard H6 = SQ::H6;
constexpr BitBoard G6 = SQ::G6;
constexpr BitBoard A7 = SQ::A7;
constexpr BitBoard B7 = SQ::B7;
constexpr BitBoard C7 = SQ::C7;
constexpr BitBoard D7 = SQ::D7;
constexpr BitBoard E7 = SQ::E7;
constexpr BitBoard F7 = SQ::F7;
constexpr BitBoard G7 = SQ::G7;
constexpr BitBoard H7 = SQ::H7;
constexpr BitBoard A8 = SQ::A8;
constexpr BitBoard B8 = SQ::B8;
constexpr BitBoard C8 = SQ::C8;
constexpr BitBoard D8 = SQ::D8;
constexpr BitBoard E8 = SQ::E8;
constexpr BitBoard F8 = SQ::F8;
constexpr BitBoard G8 = SQ::G8;
constexpr BitBoard H8 = SQ::H8;
} // namespace BB
