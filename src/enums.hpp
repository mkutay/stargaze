#pragma once
#include <array>
#include <cstdint>
#include <utility>

enum class Colour : uint8_t {
    WHITE = 0,
    BLACK = 1,
};

constexpr const static std::array<Colour, 2> COLOURS = {Colour::WHITE,
                                                        Colour::BLACK};

constexpr Colour operator!(Colour c) {
    return static_cast<Colour>(1 - std::to_underlying(c));
}

enum class Piece : uint8_t {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
};

constexpr const static std::array<Piece, 6> PIECES = {
    Piece::PAWN, Piece::KNIGHT, Piece::BISHOP,
    Piece::ROOK, Piece::QUEEN,  Piece::KING,
};
