#include <cassert>
#include "move.hpp"

u_int16_t create_move(int from, int to, int flags) { return from | (to << 6) | (flags << 12); }
u_int16_t get_move_from(u_int16_t move) { return move & 0x3f; }
u_int16_t get_move_to(u_int16_t move) { return (move >> 6) & 0x3f; }
u_int16_t get_move_flags(u_int16_t move) { return (move >> 12) & 0x0f; }
bool is_move_promotion(u_int16_t move) { return (move >> 15) & 0x01; }
bool is_move_capture(u_int16_t move) { return (move >> 14) & 0x01; }
bool is_move_en_passant(u_int16_t move) { return get_move_flags(move) == 0b0101; }
Piece get_move_promotion_piece(u_int16_t move) {
#ifdef DEBUG
  assert(is_move_promotion(move));
#endif
  int specials = get_move_flags(move) & 0b0011;
  if (specials == 0) return KNIGHT; // knight
  if (specials == 1) return BISHOP; // bishop
  if (specials == 2) return ROOK; // rook
  return QUEEN; // queen
}
std::string move_to_string(u_int16_t move) {
  std::string ret = "";
  ret += std::to_string((int) get_move_from(move)) + " ";
  ret += std::to_string((int) get_move_to(move)) + " ";
  ret += std::to_string((int) get_move_flags(move));
  return ret;
}