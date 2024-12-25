#include <random>
#include "board.hpp"
#include "get_hash.hpp"

typedef std::mt19937 MyRNG;

u_int64_t hash[12][64];
u_int64_t black_move;
u_int64_t castling[4];
u_int64_t en_passant_file[8];

int convert(Piece p, Colour c) {
  int pc = -1;
  switch (p) {
    case W_PAWN: pc = 0; break;
    case B_PAWN: pc = 0; break;
    case KNIGHT: pc = 1; break;
    case BISHOP: pc = 2; break;
    case ROOK: pc = 3; break;
    case QUEEN: pc = 4; break;
    case KING: pc = 5; break;
    case EMPTY: break;
  }
  if (c == WHITE) return pc * 2;
  return pc * 2 + 1;
}

void init_hash_table() {
  MyRNG rng;
  std::uniform_int_distribution<u_int64_t> rand; // rand(rng)
  for (int p = 0; p < 12; p++) {
    for (int i = 0; i < 64; i++) {
      hash[p][i] = rand(rng);
    }
  }
  black_move = rand(rng);
  for (int i = 0; i < 4; i++) {
    castling[i] = rand(rng);
  }
  for (int i = 0; i < 8; i++) {
    en_passant_file[i] = rand(rng);
  }
}

int Board::get_hash() {
  u_int64_t ret_hash = 0;
  for (int i = 0; i < 64; i++) {
    int pc = convert(get_piece(i), get_colour(i));
    if (!is_empty(i)) ret_hash ^= hash[pc][i];
  }
  for (int i = 0; i < 4; i++) if (can_castle[i]) 
    ret_hash ^= castling[i];
  if (turn == BLACK) ret_hash ^= black_move;
  int last_move = moves.empty() ? 0 : moves.back();
  if (get_move_flags(last_move) == 0b0001) {
    ret_hash ^= en_passant_file[get_move_to(last_move) & 7]; // mod 8
  }
  return ret_hash;
}