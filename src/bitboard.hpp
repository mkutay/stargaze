#pragma once
#include "square.hpp"
#include <cassert>
#include <concepts>
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
    template <std::unsigned_integral T>
    constexpr BitBoard operator|(T o) const {
        return bb | o;
    }
    template <std::unsigned_integral T>
    constexpr BitBoard operator&(T o) const {
        return bb & o;
    }
    template <std::unsigned_integral T>
    constexpr BitBoard operator^(T o) const {
        return bb ^ o;
    }
    constexpr BitBoard operator<<(const int s) const { return bb << s; }
    constexpr BitBoard operator>>(const int s) const { return bb >> s; }
    constexpr BitBoard operator~() const { return ~bb; }

    constexpr BitBoard north() const { return bb << 8; }
    constexpr BitBoard south() const { return bb >> 8; }
    constexpr BitBoard east() const {
        return (bb & 0x7f7f7f7f7f7f7f7full) << 1;
    }
    constexpr BitBoard west() const {
        return (bb & 0xfefefefefefefefeull) >> 1;
    }

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
    template <std::unsigned_integral T> BitBoard &operator|=(T o) {
        bb |= o;
        return *this;
    }
    template <std::unsigned_integral T> BitBoard &operator&=(T o) {
        bb &= o;
        return *this;
    }
    template <std::unsigned_integral T> BitBoard &operator^=(T o) {
        bb ^= o;
        return *this;
    }
    BitBoard &operator<<=(const int s) {
        bb <<= s;
        return *this;
    }
    BitBoard &operator>>=(const int s) {
        bb >>= s;
        return *this;
    }

    constexpr bool operator!=(const BitBoard &o) const { return bb != o.bb; }
    constexpr bool operator==(const BitBoard &o) const { return bb == o.bb; }
    template <std::unsigned_integral T> constexpr bool operator!=(T o) const {
        return bb != o;
    }
    template <std::unsigned_integral T> constexpr bool operator==(T o) const {
        return bb == o;
    }

    void set_bit(Square sq) { bb |= (1ull << sq); }
    void erase_bit(Square sq) { bb &= ~(1ull << sq); }
    void toggle_bit(Square sq) { bb ^= (1ull << sq); }

    /**
     * Checks if the bitboard has not set any of the bits in check.
     */
    constexpr bool empty(BitBoard check) const { return !(bb & check); }

    /**
     * Checks if the bitboard has set all of the bits in check.
     */
    constexpr bool occupied(BitBoard check) const {
        return (bb & check) == check;
    }

    /**
     * Return the adjacent squares.
     */
    constexpr BitBoard adjacent() const {
        return north() | south() | east() | west() | north().east() |
               north().west() | south().east() | south().west();
    }

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
constexpr auto A1 = BitBoard(SQ::A1);
constexpr auto B1 = BitBoard(SQ::B1);
constexpr auto C1 = BitBoard(SQ::C1);
constexpr auto D1 = BitBoard(SQ::D1);
constexpr auto E1 = BitBoard(SQ::E1);
constexpr auto F1 = BitBoard(SQ::F1);
constexpr auto G1 = BitBoard(SQ::G1);
constexpr auto H1 = BitBoard(SQ::H1);
constexpr auto A2 = BitBoard(SQ::A2);
constexpr auto B2 = BitBoard(SQ::B2);
constexpr auto C2 = BitBoard(SQ::C2);
constexpr auto D2 = BitBoard(SQ::D2);
constexpr auto E2 = BitBoard(SQ::E2);
constexpr auto F2 = BitBoard(SQ::F2);
constexpr auto G2 = BitBoard(SQ::G2);
constexpr auto H2 = BitBoard(SQ::H2);
constexpr auto A3 = BitBoard(SQ::A3);
constexpr auto B3 = BitBoard(SQ::B3);
constexpr auto C3 = BitBoard(SQ::C3);
constexpr auto D3 = BitBoard(SQ::D3);
constexpr auto E3 = BitBoard(SQ::E3);
constexpr auto F3 = BitBoard(SQ::F3);
constexpr auto G3 = BitBoard(SQ::G3);
constexpr auto H3 = BitBoard(SQ::H3);
constexpr auto A4 = BitBoard(SQ::A4);
constexpr auto B4 = BitBoard(SQ::B4);
constexpr auto C4 = BitBoard(SQ::C4);
constexpr auto D4 = BitBoard(SQ::D4);
constexpr auto E4 = BitBoard(SQ::E4);
constexpr auto F4 = BitBoard(SQ::F4);
constexpr auto G4 = BitBoard(SQ::G4);
constexpr auto H4 = BitBoard(SQ::H4);
constexpr auto A5 = BitBoard(SQ::A5);
constexpr auto B5 = BitBoard(SQ::B5);
constexpr auto C5 = BitBoard(SQ::C5);
constexpr auto D5 = BitBoard(SQ::D5);
constexpr auto E5 = BitBoard(SQ::E5);
constexpr auto F5 = BitBoard(SQ::F5);
constexpr auto G5 = BitBoard(SQ::G5);
constexpr auto H5 = BitBoard(SQ::H5);
constexpr auto A6 = BitBoard(SQ::A6);
constexpr auto B6 = BitBoard(SQ::B6);
constexpr auto C6 = BitBoard(SQ::C6);
constexpr auto D6 = BitBoard(SQ::D6);
constexpr auto E6 = BitBoard(SQ::E6);
constexpr auto F6 = BitBoard(SQ::F6);
constexpr auto H6 = BitBoard(SQ::H6);
constexpr auto G6 = BitBoard(SQ::G6);
constexpr auto A7 = BitBoard(SQ::A7);
constexpr auto B7 = BitBoard(SQ::B7);
constexpr auto C7 = BitBoard(SQ::C7);
constexpr auto D7 = BitBoard(SQ::D7);
constexpr auto E7 = BitBoard(SQ::E7);
constexpr auto F7 = BitBoard(SQ::F7);
constexpr auto G7 = BitBoard(SQ::G7);
constexpr auto H7 = BitBoard(SQ::H7);
constexpr auto A8 = BitBoard(SQ::A8);
constexpr auto B8 = BitBoard(SQ::B8);
constexpr auto C8 = BitBoard(SQ::C8);
constexpr auto D8 = BitBoard(SQ::D8);
constexpr auto E8 = BitBoard(SQ::E8);
constexpr auto F8 = BitBoard(SQ::F8);
constexpr auto G8 = BitBoard(SQ::G8);
constexpr auto H8 = BitBoard(SQ::H8);

static_assert(A1.adjacent() == (A2 | B1 | B2));
static_assert(H1.adjacent() == (H2 | G1 | G2));
static_assert(A8.adjacent() == (A7 | B7 | B8));
static_assert(H8.adjacent() == (G7 | G8 | H7));
static_assert(D4.adjacent() == (C3 | C4 | C5 | E3 | E4 | E5 | D3 | D5));
static_assert(A4.lsb() == SQ::A4);
static_assert(D4.lsb() == SQ::D4);
static_assert(H8.lsb() == SQ::H8);
static_assert((C3 | C4 | C5 | E3 | E4 | E5 | D3 | D5).lsb() == SQ::C3);
} // namespace BB

static_assert(sizeof(BitBoard) == sizeof(uint64_t));
