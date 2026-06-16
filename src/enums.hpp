#pragma once
#include <array>
#include <cstdint>
#include <string>
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

/**
 * Returns the weight of a colour, where white is +1 and black is -1.
 */
constexpr int weight(Colour c) { return -std::to_underlying(c) * 2 + 1; }

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

constexpr std::string piece_to_string(Piece p) {
    switch (p) {
    case Piece::PAWN:
        return "p";
    case Piece::KNIGHT:
        return "n";
    case Piece::BISHOP:
        return "b";
    case Piece::ROOK:
        return "r";
    case Piece::QUEEN:
        return "q";
    case Piece::KING:
        return "k";
    }
}
