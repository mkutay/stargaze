#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

class Square {
  private:
    /**
     * This layout is used to represent the chess board:
     *
     * 56 57 58 59 60 61 62 63
     * 48 49 50 51 52 53 54 55
     * 40 41 42 43 44 45 46 47
     * 32 33 34 35 36 37 38 39
     * 24 25 26 27 28 29 30 31
     * 16 17 18 19 20 21 22 23
     * 8  9  10 11 12 13 14 15
     * 0  1  2  3  4  5  6  7
     */
    uint8_t sq;

  public:
    /**
     * The default constructor is deleted to prevent accidentally creating an
     * uninitialised square, preserving the variant.
     */
    constexpr Square() = delete;
    constexpr Square(uint8_t sq) : sq(sq) { assert(sq >= 0 && sq <= 64); }
    constexpr Square(uint8_t rank, uint8_t file) : sq(rank * 8 + file) {
        assert(rank >= 0 && rank <= 7);
        assert(file >= 0 && file <= 7);
    }
    constexpr Square(std::string str) {
        assert(str.size() == 2);
        char file = str[0];
        char rank = str[1];
        assert(file >= 'a' && file <= 'h');
        assert(rank >= '1' && rank <= '8');
        sq = (rank - '1') * 8 + (file - 'a');
    }

    constexpr operator int() const { return sq; }

    constexpr Square operator+(const Square &o) const { return sq + o.sq; }
    constexpr Square operator-(const Square &o) const { return sq - o.sq; }
    constexpr Square operator^(const Square &o) const { return sq ^ o; }
    constexpr Square operator+(const int &o) const { return sq + o; }
    constexpr Square operator-(const int &o) const { return sq - o; }
    constexpr Square operator^(const int &o) const { return sq ^ o; }

    constexpr Square &operator+=(const Square &o) {
        sq += o.sq;
        return *this;
    }
    constexpr Square &operator-=(const Square &o) {
        sq -= o.sq;
        return *this;
    }
    constexpr Square &operator++() {
        sq++;
        return *this;
    }
    constexpr Square operator++(int) {
        Square temp = *this;
        sq++;
        return temp;
    }

    constexpr std::string to_string() {
        char file = 'a' + (sq % 8);
        char rank = '1' + (sq / 8);
        return std::string() + file + rank;
    }

    constexpr Square north() const {
        assert(rank() < 7);
        return sq + 8;
    }

    constexpr Square south() const {
        assert(rank() >= 1);
        return sq - 8;
    }

    constexpr Square west() const {
        assert(file() >= 1);
        return sq - 1;
    }

    constexpr Square east() const {
        assert(file() < 7);
        return sq + 1;
    }

    /**
     * Create a new square with a difference of `move`.
     */
    constexpr std::optional<Square> move(int8_t move) const {
        auto [rank_difference, file_difference] = decompose(move);

        auto new_rank = rank_difference + rank();
        auto new_file = file_difference + file();

        if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8)
            return Square(new_rank, new_file);
        return std::nullopt;
    }

    /**
     * Decomposes the signed offset into rank and file components using the
     * symmetric remainder (file in [-4, 4]). This correctly handles offsets
     * such as +7 (NW: rank + 1, file - 1) and -7 (SE: rank - 1, file + 1),
     * which straddle a rank boundary.
     */
    constexpr static std::pair<int8_t, int8_t> decompose(int8_t move) {
        int8_t rank_difference = move / 8;
        int8_t file_difference = move % 8;

        // Normalise file_difference into [-4, +4].
        if (file_difference > 4) {
            rank_difference += 1;
            file_difference -= 8;
        } else if (file_difference < -4) {
            rank_difference -= 1;
            file_difference += 8;
        }

        return {rank_difference, file_difference};
    }

    constexpr uint8_t file() const { return sq % 8; }
    constexpr uint8_t rank() const { return sq / 8; }
};

namespace SQ {
constexpr Square A1 = Square(0);
constexpr Square B1 = Square(1);
constexpr Square C1 = Square(2);
constexpr Square D1 = Square(3);
constexpr Square E1 = Square(4);
constexpr Square F1 = Square(5);
constexpr Square G1 = Square(6);
constexpr Square H1 = Square(7);
constexpr Square A2 = Square(8);
constexpr Square B2 = Square(9);
constexpr Square C2 = Square(10);
constexpr Square D2 = Square(11);
constexpr Square E2 = Square(12);
constexpr Square F2 = Square(13);
constexpr Square G2 = Square(14);
constexpr Square H2 = Square(15);
constexpr Square A3 = Square(16);
constexpr Square B3 = Square(17);
constexpr Square C3 = Square(18);
constexpr Square D3 = Square(19);
constexpr Square E3 = Square(20);
constexpr Square F3 = Square(21);
constexpr Square G3 = Square(22);
constexpr Square H3 = Square(23);
constexpr Square A4 = Square(24);
constexpr Square B4 = Square(25);
constexpr Square C4 = Square(26);
constexpr Square D4 = Square(27);
constexpr Square E4 = Square(28);
constexpr Square F4 = Square(29);
constexpr Square G4 = Square(30);
constexpr Square H4 = Square(31);
constexpr Square A5 = Square(32);
constexpr Square B5 = Square(33);
constexpr Square C5 = Square(34);
constexpr Square D5 = Square(35);
constexpr Square E5 = Square(36);
constexpr Square F5 = Square(37);
constexpr Square G5 = Square(38);
constexpr Square H5 = Square(39);
constexpr Square A6 = Square(40);
constexpr Square B6 = Square(41);
constexpr Square C6 = Square(42);
constexpr Square D6 = Square(43);
constexpr Square E6 = Square(44);
constexpr Square F6 = Square(45);
constexpr Square G6 = Square(46);
constexpr Square H6 = Square(47);
constexpr Square A7 = Square(48);
constexpr Square B7 = Square(49);
constexpr Square C7 = Square(50);
constexpr Square D7 = Square(51);
constexpr Square E7 = Square(52);
constexpr Square F7 = Square(53);
constexpr Square G7 = Square(54);
constexpr Square H7 = Square(55);
constexpr Square A8 = Square(56);
constexpr Square B8 = Square(57);
constexpr Square C8 = Square(58);
constexpr Square D8 = Square(59);
constexpr Square E8 = Square(60);
constexpr Square F8 = Square(61);
constexpr Square G8 = Square(62);
constexpr Square H8 = Square(63);
}; // namespace SQ

static_assert(sizeof(Square) == sizeof(uint8_t));
