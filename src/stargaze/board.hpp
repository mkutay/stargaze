#pragma once
#include "stargaze/bitboard.hpp"
#include "stargaze/colour.hpp"
#include "stargaze/move.hpp"
#include "stargaze/piece.hpp"
#include "stargaze/square.hpp"
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Board {
    std::array<BitBoard, 6> piece_bbs;
    std::array<BitBoard, 2> colour_bbs;
    std::vector<Move> moves;
    Colour turn;

    // 0: white king's side, 1: white queen's side,
    // 2: black king's side, 3: black queen's side
    std::array<bool, 4> can_castle;

    /**
     * The ep_square is the square that can be captured en passant, if any;
     * i.e., the square right behind a double pawn push.
     */
    std::optional<Square> ep_square;

    /**
     * Number of half moves made since the last pawn move or capture, used for
     * the fifty-move rule.
     */
    uint8_t halfmove_clock;

    /**
     * Number of full moves made in the game. This is incremented after Black's
     * move.
     */
    uint16_t fullmove_number;

    /**
     * The hash of the current board state, used for transposition table lookup.
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

    struct UndoInfo {
        Piece moving_piece;
        std::optional<Piece> captured_piece;
        std::array<bool, 4> can_castle;
        std::optional<Square> ep_square;
        uint8_t halfmove_clock;
    };

    std::vector<UndoInfo> history;
    std::vector<uint64_t> hash_history;

    BitBoard &get_bb(Piece type);
    BitBoard get_bb(Piece type) const;
    BitBoard &get_bb(Colour colour);
    BitBoard get_bb(Colour colour) const;
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
     * Return true if the square (a single-bit bitboard) is attacked by the
     * opponent of `by_colour`.
     */
    bool is_attacked(Colour by_colour, BitBoard bb) const;

    uint64_t calculate_hash() const;

    void initialise_eval(std::array<int, 2> &mg_score,
                         std::array<int, 2> &eg_score, int &game_phase) const;

    /**
     * Check that the board state is consistent, i.e., that the current hash
     * matches the actual board state, evaluation data is correct, and that the
     * bitboards are consistent with each other. This is useful for debugging
     * and testing.
     */
    void check_state_consistency() const;

    /**
     * Apply a move to the board, updating the board state accordingly. This
     * includes updating the pieces, castling rights, and move history.
     *
     * Note that we assume the move is valid and legal. That is, we don't check
     * if the move is actually possible, such as moving a piece that isn't
     * there, or moving to a square occupied by your own piece, or moving into
     * check. We also don't check if the move is legal in terms of the rules of
     * chess, such as castling through check or en passant when not possible.
     */
    template <bool Undo> void apply_move(Move move);

    /**
     * Returns a bitboard of all squares attacked by the given colour.
     */
    BitBoard attacked(Colour attacker) const;

    /**
     * Returns the pieces of the given colour attacking a square.
     */
    BitBoard attackers(Colour attacker, Square square) const;

    /**
     * Returns the squares that can capture or block a lone checker.
     */
    BitBoard check_evasion_targets(Square king, BitBoard checkers) const;

    /**
     * Returns the opposing sliders pinning the given colour to a square.
     */
    BitBoard pinners(Colour colour, Square square) const;

    /**
     * Returns the ray on which a pinned piece may move.
     */
    static BitBoard pin_ray(Square pivot, Square piece, BitBoard pinners);

    /**
     * Returns a bitboard of all squares pinned by the given colour, with the
     * given square as the pivot.
     */
    BitBoard pinned(Colour colour, Square square) const;

  public:
    Board();
    explicit Board(std::string_view fen);

    uint64_t perft(int depth);
    void make_move(Move move);
    void undo_move();
    void make_null_move();
    void undo_null_move();
    template <bool CapturesOnly = false> std::vector<Move> get_moves();
    uint64_t get_hash() const;
    bool is_in_check(Colour by_colour) const;

    /**
     * Check if the given colour has any non-pawn, non-king material.
     */
    bool has_non_pawn_material(Colour colour) const;

    /**
     * Return the evaluation score of the board from the perspective of the
     * current player.
     */
    int evaluate() const;

    /**
     * Return a vertically mirrored version of the board.
     */
    Board mirrored() const;

    std::optional<Piece> get_piece(Square sq) const;
    std::optional<Colour> get_colour(Square sq) const;
    Colour get_turn() const;
    const std::vector<Move> get_move_history() const;
    const std::array<bool, 4> get_castling_rights() const;
    std::string nice() const;
    std::string fen() const;
    bool is_draw() const;
    bool is_repetition() const;
};
