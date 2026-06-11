#pragma once
#include <cassert>
#include <cstdint>
#include <utility>

/**
 * These WHITE and BLACK states are useful to only
 * represent colours where necessary.
 */
enum class Piece : uint8_t {
    W_PAWN = 0,
    W_KNIGHT = 1,
    W_BISHOP = 2,
    W_ROOK = 3,
    W_QUEEN = 4,
    W_KING = 5,
    WHITE = 6, // any white piece
    B_PAWN = 7,
    B_KNIGHT = 8,
    B_BISHOP = 9,
    B_ROOK = 10,
    B_QUEEN = 11,
    B_KING = 12,
    BLACK = 13, // any black piece
    EMPTY = 14,
};

/**
 * Reverse the colour of a piece. For example, `!W_PAWN = B_PAWN`
 * and `!B_KING = W_KING`.
 */
constexpr Piece operator!(const Piece &piece) {
    assert(piece != Piece::EMPTY);

    auto underlying = std::to_underlying(piece);
    return static_cast<Piece>((underlying + 7) % 14);
}

/**
 * Return the colour of a piece. For example, `get_piece_colour(W_PAWN) =
 * WHITE`.
 */
constexpr Piece get_piece_colour(const Piece &piece) {
    assert(piece != Piece::EMPTY);
    return static_cast<Piece>((std::to_underlying(piece) / 7) * 7 + 6);
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
 *
 * The `check_colour` template parameter controls whether the function should
 * check for the WHITE and BLACK pieces. If `check_colour` is false, then the
 * function will not check for WHITE and BLACK and will instead return the piece
 * type bitboard piece.
 */
template <bool check_colour = true>
constexpr BBPiece get_bb_piece(const Piece &piece) {
    assert(piece != Piece::EMPTY);

    auto underlying = std::to_underlying(piece);

    if constexpr (check_colour) {
        // For converting WHITE and BLACK.
        if (underlying == 6)
            return BBPiece::WHITE;
        if (underlying == 13)
            return BBPiece::BLACK;
    }

    return static_cast<BBPiece>((underlying % 7) + 2);
}

/**
 * Get the colour bitboard piece for a piece. For example,
 * `get_piece_colour_bb(Piece::W_PAWN) = BBPiece::WHITE` and
 * `get_piece_colour_bb(Piece::B_KING) = BBPiece::BLACK`.
 */
constexpr BBPiece get_piece_colour_bb(const Piece &piece) {
    assert(piece != Piece::EMPTY);
    return static_cast<BBPiece>((std::to_underlying(piece) / 7));
}
