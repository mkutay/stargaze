#pragma once
#include <cstdint>
#include <utility>

enum class Colour : uint8_t {
    WHITE = 0,
    BLACK = 1,
};

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
