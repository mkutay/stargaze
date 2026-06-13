#pragma once
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "bitboard.hpp"
#include "enums.hpp"
#include "move.hpp"
#include "square.hpp"

class Board {
    // Bitboard for each piece, indexed by Piece (PAWN=0..KING=5).
    std::array<BitBoard, 6> piece_bbs;

    // Bitboard for each colour, indexed by Colour (WHITE=0, BLACK=1).
    std::array<BitBoard, 2> colour_bbs;

    // 0: white king's side, 1: white queen's side,
    // 2: black king's side, 3: black queen's side
    std::array<bool, 4> can_castle;

    /**
     * Moves made in the game so far, in order. This is used for undoing moves.
     */
    std::vector<Move> moves;

    /**
     * Colour of the player whose turn it is to move.
     */
    Colour turn;

    struct BoardSnapshot {
        std::array<BitBoard, 6> piece_bbs;
        std::array<BitBoard, 2> colour_bbs;
    };

    std::vector<BoardSnapshot> board_history;
    std::vector<std::array<bool, 4>> castle_history;

    /**
     * Get the bitboard for a given piece type, as an lvalue reference so it
     * can be modified directly.
     */
    BitBoard &get_bb(Piece type);
    BitBoard get_bb(Piece type) const;

    /**
     * Get the bitboard for a given colour, as an lvalue reference so it can
     * be modified directly.
     */
    BitBoard &get_bb(Colour colour);
    BitBoard get_bb(Colour colour) const;

    /**
     * Return the bitboard for a specific coloured piece type (e.g., white
     * pawns).
     */
    BitBoard get_bb(Piece type, Colour colour) const;

    /**
     * Make a move from the from square to the to square, updating the bitboards
     * accordingly. If is_capture is true, also clear the piece on the to
     * square.
     *
     * Note: There must be a piece on the from square, and if is_capture is
     * true, there must be a piece on the to square, otherwise the behavior is
     * undefined.
     */
    template <bool is_capture> void make_move_bb(Square from, Square to);

    /**
     * Return the piece type and colour occupying sq, or nullopt if the
     * square is empty.
     */
    std::optional<std::pair<Piece, Colour>> get_piece_colour(Square sq) const;

    /**
     * Return the piece type occupying sq, or nullopt if the square is
     * empty.
     */
    std::optional<Piece> get_piece(Square sq) const;

    /**
     * Return the colour occupying sq, or nullopt if the square is empty.
     */
    std::optional<Colour> get_colour(Square sq) const;

    /**
     * Return true if the piece of the given type and colour occupies the
     * square.
     *
     * The Square to BitBoard implicit conversion can be utilised here.
     *
     * On any square set on the bitboard, the type and the colour match, still
     * returns true.
     */
    bool has_piece_at(BitBoard bb, Piece type, Colour colour) const;

    bool is_attacked(Colour by_colour, BitBoard bb);

  public:
    Board();
    void make_move(Move move);
    void undo_move();
    Colour get_turn() const;
    std::vector<Move> get_moves();
    std::string to_string() const;
    int get_hash() const;
    bool is_in_check(Colour by_colour);

    const std::vector<Move> get_move_history() const;
    const std::array<bool, 4> get_castling_rights() const;
    int evaluate() const;
};
