#pragma once
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "enums.hpp"
#include "move.hpp"

class Board {
    // Bitboard for each piece type, indexed by Piece (PAWN=0..KING=5).
    std::array<uint64_t, 6> type_bbs = {};

    // Bitboard for each colour, indexed by Colour (WHITE=0, BLACK=1).
    std::array<uint64_t, 2> colour_bbs = {};

    // 0: white king's side, 1: white queen's side,
    // 2: black king's side, 3: black queen's side
    std::array<bool, 4> can_castle = {true, true, true, true};

    /**
     * Moves made in the game so far, in order. This is used for undoing moves
     * and also for keeping track of whose turn it is.
     *
     * In particular, it's white's turn if moves.size() is even.
     */
    std::vector<Move> moves;

    struct BoardSnapshot {
        std::array<uint64_t, 6> types;
        std::array<uint64_t, 2> colours;
    };
    std::vector<BoardSnapshot> board_history;
    std::vector<std::array<bool, 4>> castle_history;

    /**
     * Get the bitboard for a given piece type, as an lvalue reference so it
     * can be modified directly.
     */
    uint64_t &get_bb(Piece type);
    uint64_t get_bb(Piece type) const;

    /**
     * Get the bitboard for a given colour, as an lvalue reference so it can
     * be modified directly.
     */
    uint64_t &get_bb(Colour colour);
    uint64_t get_bb(Colour colour) const;

  public:
    Board();
    void make_move(Move move);
    void undo_move();
    Colour get_turn();
    std::vector<Move> get_moves();
    std::string to_string();
    int get_hash();
    bool is_in_check(Colour by_colour);
    bool is_attacked(Colour by_colour,
                     std::variant<unsigned int, uint64_t> square);

    /**
     * Return the piece type and colour occupying sq, or nullopt if the
     * square is empty.
     */
    std::optional<std::pair<Piece, Colour>> get_piece_colour(int sq) const;

    /**
     * Return the piece type occupying sq, or nullopt if the square is
     * empty.
     */
    std::optional<Piece> get_piece(int sq) const;

    /**
     * Return the colour occupying sq, or nullopt if the square is empty.
     */
    std::optional<Colour> get_colour(int sq) const;

    /**
     * Return true if the piece of the given type and colour occupies square i.
     */
    bool has_piece_at(int sq, Piece type, Colour colour) const;

    void make_move_bb(int from, int to, bool is_capture);
    void clear(int i);
    void set_piece(int i, Piece type, Colour colour);

    /**
     * Return the bitboard for a specific coloured piece type (e.g. white
     * pawns).
     */
    uint64_t get_piece_bb(Piece type, Colour colour) const;

    /**
     * Return the bitboard for all pieces of the given colour.
     */
    uint64_t get_colour_bb(Colour colour) const;

    /**
     * Return the bitboard of empty squares.
     */
    uint64_t get_empty_bb() const;

    const std::vector<Move> get_move_history() const;
    const std::array<bool, 4> get_castling_rights() const;
    int evaluate();
};
