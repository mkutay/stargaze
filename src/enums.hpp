#pragma once
#include <cstdint>

enum class Piece : uint8_t {
    EMPTY = 0,
    W_PAWN = 1, // white pawn
    B_PAWN = 2, // black pawn
    KNIGHT = 3,
    BISHOP = 4,
    ROOK = 5,
    QUEEN = 6,
    KING = 7,
};

enum class Colour : bool {
    WHITE = 0,
    BLACK = 1,
};

constexpr Colour operator!(const Colour& a) {
    if (a == Colour::WHITE)
        return Colour::BLACK;
    return Colour::WHITE;
}

// bit board representation (dense)
enum class BBPiece : uint8_t {
    WHITE = 0, // any white piece
    BLACK = 1, // any black piece
    PAWN = 2,
    KNIGHT = 3,
    BISHOP = 4,
    ROOK = 5,
    QUEEN = 6,
    KING = 7,
};
