#pragma once
#include <cassert>
#include <cstdint>
#include <utility>

enum class Piece : uint8_t {
    W_PAWN = 0,
    W_KNIGHT = 1,
    W_BISHOP = 2,
    W_ROOK = 3,
    W_QUEEN = 4,
    W_KING = 5,
    B_PAWN = 6,
    B_KNIGHT = 7,
    B_BISHOP = 8,
    B_ROOK = 9,
    B_QUEEN = 10,
    B_KING = 11,
    EMPTY = 12,
    /**
     * These WHITE and BLACK states are useful to only
     * represent colours where necessary.
     */
    WHITE = 13, // any white piece
    BLACK = 14, // any black piece
};

/**
 * Reverse the colour of a piece. For example, `!W_PAWN = B_PAWN`
 * and `!B_KING = W_KING`.
 */
constexpr Piece operator!(const Piece &piece) {
    assert(piece != Piece::EMPTY);

    auto underlying = std::to_underlying(piece);
    return static_cast<Piece>((underlying + 6) % 12);
}

/**
 * Return the colour of a piece. For example, `get_colour(W_PAWN) = WHITE`.
 */
constexpr Piece get_piece_colour(const Piece &piece) {
    assert(piece != Piece::EMPTY && piece != Piece::WHITE &&
           piece != Piece::BLACK);
    return static_cast<Piece>(std::to_underlying(piece) / 6 + 13);
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

/**
 * Get the corresponding bitboard piece for a piece. For example,
 * `get_bb_piece(Piece::W_PAWN) = BBPiece::PAWN`.
 *
 * It can also be used to get the colour bitboard piece, for example
 * `get_bb_piece(Piece::WHITE) = BBPiece::WHITE`.
 */
constexpr BBPiece get_bb_piece(const Piece &piece) {
    assert(piece != Piece::EMPTY);

    auto underlying = std::to_underlying(piece);

    // For converting WHITE and BLACK.
    if (underlying >= 13)
        return static_cast<BBPiece>(underlying - 13);

    return static_cast<BBPiece>((underlying % 6) + 2);
}
