#pragma once
#include "colour.hpp"
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>

class Piece {
  private:
    uint8_t piece;

  public:
    constexpr const static uint8_t PAWN = 0;
    constexpr const static uint8_t KNIGHT = 1;
    constexpr const static uint8_t BISHOP = 2;
    constexpr const static uint8_t ROOK = 3;
    constexpr const static uint8_t QUEEN = 4;
    constexpr const static uint8_t KING = 5;

    constexpr Piece() : piece(PAWN) {}
    constexpr Piece(int _piece) : piece(_piece) {}

    /**
     * Colour be capital or not, returns the piece accordingly.
     */
    constexpr Piece(char c) {
        switch (std::tolower(c)) {
        case 'p':
            piece = Piece::PAWN;
            break;
        case 'n':
            piece = Piece::KNIGHT;
            break;
        case 'b':
            piece = Piece::BISHOP;
            break;
        case 'r':
            piece = Piece::ROOK;
            break;
        case 'q':
            piece = Piece::QUEEN;
            break;
        case 'k':
            piece = Piece::KING;
            break;
        default:
            std::unreachable();
        }
    }

    constexpr operator uint8_t() const { return piece; }

    constexpr Piece operator+(const Piece &o) const { return piece + o.piece; }
    constexpr Piece operator-(const Piece &o) const { return piece - o.piece; }
    constexpr Piece operator+(const uint8_t &o) const { return piece + o; }
    constexpr Piece operator-(const uint8_t &o) const { return piece - o; }

    constexpr Piece &operator+=(const Piece &other) {
        piece += other.piece;
        return *this;
    }
    constexpr Piece &operator-=(const Piece &other) {
        piece -= other.piece;
        return *this;
    }
    constexpr Piece &operator++() {
        piece++;
        return *this;
    }
    constexpr Piece operator++(int) {
        Piece temp = *this;
        piece++;
        return temp;
    }

    constexpr std::string to_string() const {
        switch (piece) {
        case PAWN:
            return "p";
        case KNIGHT:
            return "n";
        case BISHOP:
            return "b";
        case ROOK:
            return "r";
        case QUEEN:
            return "q";
        case KING:
            return "k";
        }

        std::unreachable();
    }

    constexpr std::string nice(Colour colour) const {
        std::array<std::string, 2> piece_str;
        switch (piece) {
        case PAWN:
            piece_str = {"♟", "♙"};
            break;
        case KNIGHT:
            piece_str = {"♞", "♘"};
            break;
        case BISHOP:
            piece_str = {"♝", "♗"};
            break;
        case ROOK:
            piece_str = {"♜", "♖"};
            break;
        case QUEEN:
            piece_str = {"♛", "♕"};
            break;
        case KING:
            piece_str = {"♚", "♔"};
            break;
        default:
            std::unreachable();
        }

        return piece_str[colour];
    }
};

namespace PP {
constexpr auto PAWN = Piece(Piece::PAWN);
constexpr auto KNIGHT = Piece(Piece::KNIGHT);
constexpr auto BISHOP = Piece(Piece::BISHOP);
constexpr auto ROOK = Piece(Piece::ROOK);
constexpr auto QUEEN = Piece(Piece::QUEEN);
constexpr auto KING = Piece(Piece::KING);

static_assert(PAWN < KNIGHT);
static_assert(KNIGHT < BISHOP);
static_assert(BISHOP < ROOK);
static_assert(ROOK < QUEEN);
static_assert(QUEEN < KING);
static_assert(PAWN.nice(CC::WHITE) == "♟");
static_assert(PAWN.nice(CC::BLACK) == "♙");
static_assert(KNIGHT.nice(CC::WHITE) == "♞");
static_assert(KNIGHT.nice(CC::BLACK) == "♘");
static_assert(BISHOP.nice(CC::WHITE) == "♝");
static_assert(BISHOP.nice(CC::BLACK) == "♗");
static_assert(ROOK.nice(CC::WHITE) == "♜");
static_assert(ROOK.nice(CC::BLACK) == "♖");
static_assert(QUEEN.nice(CC::WHITE) == "♛");
static_assert(QUEEN.nice(CC::BLACK) == "♕");
static_assert(KING.nice(CC::WHITE) == "♚");
static_assert(KING.nice(CC::BLACK) == "♔");
static_assert(PAWN.to_string() == "p");
static_assert(KNIGHT.to_string() == "n");
static_assert(BISHOP.to_string() == "b");
static_assert(ROOK.to_string() == "r");
static_assert(QUEEN.to_string() == "q");
static_assert(KING.to_string() == "k");
} // namespace PP

constexpr const static std::array<Piece, 6> PIECES = {
    PP::PAWN, PP::KNIGHT, PP::BISHOP, PP::ROOK, PP::QUEEN, PP::KING,
};

static_assert(sizeof(Piece) == sizeof(uint8_t));
