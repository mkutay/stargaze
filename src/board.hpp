#pragma once
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
  std::vector<u_int16_t> moves;
  std::vector<u_int64_t *> board_history;

  bool debug_print(u_int16_t move);
  bool debug_print();
  int piece_code(Piece p) {
    switch (p) {
      case W_PAWN: return 2;
      case B_PAWN: return 2;
      case KNIGHT: return 3;
      case BISHOP: return 4;
      case ROOK: return 5;
      case QUEEN: return 6;
      case KING: return 7;
      case EMPTY: assert(false || debug_print()); return -1;
    }
  }
  int colour_code(Piece p) { return p >= 6 ? 1 : 0; }
public:
  Board();
  void make_move(u_int16_t move);
  void undo_move();
  Colour get_turn() { return turn; }
  std::vector<u_int16_t> get_moves();
  std::string to_string();
  bool is_king_missing() { return __builtin_popcountll(pieceBB[nKing]) != 2; }
  int get_hash();

  Colour get_colour(int i) { return pieceBB[nWhite] & (1ull << i) ? WHITE : BLACK; }
  Piece get_piece(int i) {
    if (pieceBB[nPawn] & (1ull << i)) return pieceBB[nWhite] & (1ull << i) ? W_PAWN : B_PAWN;
    if (pieceBB[nKnight] & (1ull << i)) return KNIGHT;
    if (pieceBB[nBishop] & (1ull << i)) return BISHOP;
    if (pieceBB[nRook] & (1ull << i)) return ROOK;
    if (pieceBB[nQueen] & (1ull << i)) return QUEEN;
    if (pieceBB[nKing] & (1ull << i)) return KING;
    return EMPTY;
  }
  void make_move_bb(int from, int to, bool is_capture) {
    Piece p = get_piece(from), cp = get_piece(to);
    Colour c = get_colour(from), cc = get_colour(to);
    u_int64_t fromBB = 1ull << from;
    u_int64_t toBB = 1ull << to;
    u_int64_t bb = fromBB | toBB;
    pieceBB[piece_code(p)] ^= bb;
    pieceBB[c] ^= bb;
    if (is_capture && cp != EMPTY) pieceBB[piece_code(cp)] ^= toBB, pieceBB[cc] ^= toBB;
  }
  void clear(int i) { for (int j = 0; j < 8; j++) pieceBB[j] &= ~(1ull << i); }
  bool is_empty(int i) { return !((pieceBB[nWhite] | pieceBB[nBlack]) & (1ull << i)); }
  void set_piece(int i, Piece p, Colour c) {
    clear(i);
    pieceBB[c == WHITE ? nWhite : nBlack] |= (1ull << i);
    pieceBB[piece_code(p)] |= (1ull << i);
  }

  u_int64_t get_white_pieces() { return pieceBB[nWhite]; }
  u_int64_t get_black_pieces() { return pieceBB[nBlack]; }
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