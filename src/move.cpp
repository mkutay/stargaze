#include <cassert>
#include "move.h"

Move::Move(int from, int to, int flags) {
  m_move = from | (to << 6) | (flags << 12);
}

unsigned short Move::get_from() { return m_move & 0x3f; }
unsigned short Move::get_to() { return (m_move >> 6) & 0x3f; }
unsigned short Move::get_flags() { return (m_move >> 12) & 0x0f; }
bool Move::is_promotion() { return (m_move >> 15) & 0x01; }
bool Move::is_capture() { return (m_move >> 14) & 0x01; }
Piece Move::get_promotion_piece() {
  if (!is_promotion()) assert(false);
  int specials = get_flags() & 0b0011;
  if (specials == 0) return KNIGHT; // knight
  if (specials == 1) return BISHOP; // bishop
  if (specials == 2) return ROOK; // rook
  return QUEEN; // queen
}

// std::string Move::to_string() {
//   std::string ret = "";
//   ret += (char)('a' + get_from() % 8);
//   ret += (char)('8' - get_from() / 8);
//   ret += (char)('a' + get_to() % 8);
//   ret += (char)('8' - get_to() / 8);
//   if (is_promotion()) {
//     ret += '=';
//     int piece = get_promotion_piece();
//     if (piece == 2) ret += 'N';
//     if (piece == 3) ret += 'B';
//     if (piece == 4) ret += 'R';
//     if (piece == 5) ret += 'Q';
//   }
//   return ret;
// }

std::string Move::to_string() {
  std::string ret = "";
  ret += std::to_string((int) get_from()) + " ";
  ret += std::to_string((int) get_to()) + " ";
  ret += std::to_string((int) get_flags());
  return ret;
}