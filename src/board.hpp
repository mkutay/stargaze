#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "move.hpp"

/**
 * bit board representation (dense):
 * 0: any white piece
 * 1: any black piece
 * 2: pawns
 * 3: knights
 * 4: bishops
 * 5: rooks
 * 6: queens
 * 7: kings
 */

enum enumBBPiece {
  nWhite, // any white piece
  nBlack, // any black piece
  nPawn,
  nKnight,
  nBishop,
  nRook,
  nQueen,
  nKing,
};

class Board {
  u_int64_t pieceBB[8]; // unsigned long long
  Colour turn = WHITE;
  bool can_castle[4] = { true }; // 0: white king's side, 1: white queen's side, 2: black king's side, 3: black queen's side
  std::vector<Move *> moves;
  std::vector<u_int64_t *> board_history;
  static int mg_table[12][64], eg_table[12][64], gamephase_inc[12], mg_value[6], eg_value[6],
    *mg_pesto_table[6], *eg_pesto_table[6],
    mg_pawn_table[64], eg_pawn_table[64], mg_knight_table[64], eg_knight_table[64],
    mg_bishop_table[64], eg_bishop_table[64], mg_rook_table[64], eg_rook_table[64],
    mg_queen_table[64], eg_queen_table[64], mg_king_table[64], eg_king_table[64];

  bool debug_print(Move *move);
  int piece_code(Piece p) {
    switch (p) {
      case W_PAWN: return 2;
      case B_PAWN: return 2;
      case KNIGHT: return 3;
      case BISHOP: return 4;
      case ROOK: return 5;
      case QUEEN: return 6;
      case KING: return 7;
      case EMPTY: assert(false); break;
    }
  }
  int colour_code(Piece p) { return p >= 6 ? 1 : 0; }
  void init_eval_table();
public:
  Board();
  void make_move(Move *move);
  void undo_move();
  bool get_turn() { return turn; }
  int evaluate();
  std::vector<Move *> get_moves();
  std::string to_string();

  Colour get_colour(int i) { return pieceBB[nWhite] & (1ull << i) ? WHITE : BLACK; }
  Piece get_piece(int i) {
    if (pieceBB[nPawn] & (1ull << i)) return get_colour(i) == WHITE ? W_PAWN : B_PAWN;
    if (pieceBB[nKnight] & (1ull << i)) return KNIGHT;
    if (pieceBB[nBishop] & (1ull << i)) return BISHOP;
    if (pieceBB[nRook] & (1ull << i)) return ROOK;
    if (pieceBB[nQueen] & (1ull << i)) return QUEEN;
    if (pieceBB[nKing] & (1ull << i)) return KING;
    return EMPTY;
  }
  void copy(int to, int from, bool is_capture, Colour c) { // board[i1] = board[i2], board[i2] = 0
    if (c == WHITE) {
      pieceBB[nWhite] |= (1ull << to);
      if (is_capture) pieceBB[nBlack] &= ~(1ull << to);
    } else {
      pieceBB[nBlack] |= (1ull << to);
      if (is_capture) pieceBB[nWhite] &= ~(1ull << to);
    }
    for (int i = 2; i < 8; i++) {
      if (pieceBB[i] & (1ull << from)) pieceBB[i] |= (1ull << to);
    }
    clear(from);
  }
  void clear(int i) { for (int j = 0; j < 8; j++) pieceBB[j] &= ~(1ull << i); }
  bool is_empty(int i) { return !((pieceBB[nWhite] | pieceBB[nBlack]) & (1ull << i)); }
  void set_piece(int i, Piece p, Colour c) {
    clear(i);
    pieceBB[c == WHITE ? nWhite : nBlack] |= (1ull << i);
    pieceBB[piece_code(p)] |= (1ull << i);
  }

  u_int64_t get_white_pawns() { return pieceBB[nPawn] & pieceBB[nWhite]; }
  u_int64_t get_black_pawns() { return pieceBB[nPawn] & pieceBB[nBlack]; }
  u_int64_t get_white_knights() { return pieceBB[nKnight] & pieceBB[nWhite]; }
  u_int64_t get_black_knights() { return pieceBB[nKnight] & pieceBB[nBlack]; }
  u_int64_t get_white_bishops() { return pieceBB[nBishop] & pieceBB[nWhite]; }
  u_int64_t get_black_bishops() { return pieceBB[nBishop] & pieceBB[nBlack]; }
  u_int64_t get_white_rooks() { return pieceBB[nRook] & pieceBB[nWhite]; }
  u_int64_t get_black_rooks() { return pieceBB[nRook] & pieceBB[nBlack]; }
  u_int64_t get_white_queens() { return pieceBB[nQueen] & pieceBB[nWhite]; }
  u_int64_t get_black_queens() { return pieceBB[nQueen] & pieceBB[nBlack]; }
  u_int64_t get_white_king() { return pieceBB[nKing] & pieceBB[nWhite]; }
  u_int64_t get_black_king() { return pieceBB[nKing] & pieceBB[nBlack]; }
};