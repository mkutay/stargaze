#pragma once
#include <array>
#include <cassert>
#include <optional>
#include <string>
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

    struct UndoInfo {
        Piece moving_piece;
        std::optional<Piece> captured_piece;
        std::array<bool, 4> can_castle;
    };

    std::vector<UndoInfo> history;

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
     * Move a piece of the given type and colour from one square to another,
     * updating the bitboards, hash, and evaluation scores accordingly.
     *
     * Note the piece type and colour must match the piece on the from square,
     * otherwise the behaviour is undefined.
     */
    void move_piece(Piece piece, Colour colour, Square from, Square to);

    /**
     * Add a piece of the given type and colour to the given square, updating
     * the bitboards, hash, and evaluation scores accordingly.
     */
    void add_piece(Piece piece, Colour colour, Square sq);

    /**
     * Remove a piece of the given type and colour from the given square,
     * updating the bitboards, hash, and evaluation scores accordingly.
     *
     * Note the piece type and colour must match the piece on the square,
     * otherwise the behaviour is undefined.
     */
    void clear_piece(Piece piece, Colour colour, Square sq);

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

    /**
     * Return true if the square is attacked by any piece of the given colour.
     * The square is represented as a bitboard with a single bit set.
     */
    bool is_attacked(Colour by_colour, BitBoard bb) const;

    /**
     * The hash of the current board state, used for transposition table looku.
     * Updated incrementally on each move.
     */
    uint64_t current_hash;

    /**
     * The evaluation score of the board in the middle game and end game, for
     * each colour. The final evaluation is a linear interpolation of the two
     * scores based on the game phase.
     */
    std::array<int, 2> mg_score;
    std::array<int, 2> eg_score;
    int game_phase;

    /**
     * Calculate the Zobrist hash of the current board state from scratch.
     */
    uint64_t calculate_hash() const;

    /**
     * Initialise the evaluation scores and game phase based on the current
     * board state.
     */
    void initialise_eval(std::array<int, 2> &mg_score,
                         std::array<int, 2> &eg_score, int &game_phase) const;

    /**
     * Check that the board state is consistent, i.e., that the current hash
     * matches the actual board state, evaluation data is correct, and that the
     * bitboards are consistent with each other. This is useful for debugging
     * and testing.
     */
    void check_state_consistency() const;

  public:
    Board();
    void make_move(Move move);
    void undo_move();
    Colour get_turn() const;
    std::vector<Move> get_moves();
    std::string to_string() const;
    uint64_t get_hash() const;

    /**
     * Return true if the king of the given colour is in check. This is done by
     * checking if the king's square is attacked by any of the opponent's
     * pieces.
     */
    bool is_in_check(Colour by_colour) const;

    const std::vector<Move> get_move_history() const;
    const std::array<bool, 4> get_castling_rights() const;
    int evaluate() const;
};
