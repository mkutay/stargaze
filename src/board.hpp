#pragma once
#include <array>
#include <cassert>
#include <string>
#include <variant>
#include <vector>

#include "enums.hpp"
#include "move.hpp"

class Board {
    // Bitboard representation of the board, indexed by BBPiece.
    std::array<uint64_t, 8> pieces = {0, 0, 0, 0, 0, 0, 0, 0};

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

    std::vector<std::array<uint64_t, 8>> board_history;
    std::vector<std::array<bool, 4>> castle_history;

    /**
     * Get the bitboard for a given piece type.
     *
     * Returns an lvalue reference so that it can be modified directly.
     */
    uint64_t &get_bb(BBPiece piece) {
        return pieces[std::to_underlying(piece)];
    }

    uint64_t get_bb(BBPiece piece) const {
        return pieces[std::to_underlying(piece)];
    }

  public:
    Board();
    void make_move(Move move);
    void undo_move();
    Piece get_turn();
    std::vector<Move> get_moves();
    std::string to_string();
    int get_hash();
    bool is_in_check(Piece by_colour);
    bool is_attacked(Piece by_colour,
                     std::variant<unsigned int, uint64_t> square);

    Piece get_colour(int i) const;
    Piece get_piece(int i) const;
    void make_move_bb(int from, int to, bool is_capture);
    void clear(int i);
    bool is_empty(int i);
    void set_piece(int i, Piece p);

    std::array<uint64_t, 8> get_all_pieces() const;
    std::array<bool, 4> get_castle_rights() const;

    uint64_t get_piece_bb(Piece piece) const;
};
