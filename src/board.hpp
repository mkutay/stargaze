#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include <variant>

#include "move.hpp"

// bit board representation (dense):
enum BBPiece {
    n_white = 0, // any white piece
    n_black = 1, // any black piece
    n_pawn = 2,
    n_knight = 3,
    n_bishop = 4,
    n_rook = 5,
    n_queen = 6,
    n_king = 7,
};

class Board {
    std::array<u_int64_t, 8> pieces = {0}; // unsigned long long
    Colour turn = WHITE;
    std::array<bool, 4> can_castle = { true, true, true, true }; // 0: white king's side, 1: white queen's side, 2: black king's side, 3: black queen's side
    std::vector<Move> moves;
    std::vector<std::array<u_int64_t, 8>> board_history;
    std::vector<std::array<bool, 4>> castle_history;

    void debug_print();
    int piece_code(Piece p);
public:
    Board();
    void make_move(Move move);
    void undo_move();
    Colour get_turn() { return turn; }
    std::vector<Move> get_moves();
    std::string to_string();
    int get_hash();
    bool is_in_check(Colour by_colour);
    bool is_attacked(Colour by_colour, std::variant<unsigned int, u_int64_t> square);

    Colour get_colour(int i);
    Piece get_piece(int i);
    void make_move_bb(int from, int to, bool is_capture);
    void clear(int i);
    bool is_empty(int i);
    void set_piece(int i, Piece p, Colour c);

    u_int64_t get_white_pieces();
    u_int64_t get_black_pieces();
    u_int64_t get_white_pawns();
    u_int64_t get_black_pawns();
    u_int64_t get_white_knights();
    u_int64_t get_black_knights();
    u_int64_t get_white_bishops();
    u_int64_t get_black_bishops();
    u_int64_t get_white_rooks();
    u_int64_t get_black_rooks();
    u_int64_t get_white_queens();
    u_int64_t get_black_queens();
    u_int64_t get_white_king();
    u_int64_t get_black_king();
    u_int64_t get_knights();
    u_int64_t get_bishops();
    u_int64_t get_rooks();
    u_int64_t get_queens();
    u_int64_t get_kings();
    u_int64_t get_pawns();
    std::array<u_int64_t, 8> get_all_pieces() const;
};