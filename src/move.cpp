#include "move.h"

Move::Move(int from, int to, int flags) {
  m_move = from | (to << 6) | (flags << 12);
}

unsigned short Move::get_from() { return m_move & 0x3f; }
unsigned short Move::get_to() { return (m_move >> 6) & 0x3f; }
unsigned short Move::get_flags() { return (m_move >> 12) & 0x0f; }
bool Move::is_promotion() { return (m_move >> 12) & 0x01; }
bool Move::is_capture() { return (m_move >> 13) & 0x01; }
int Move::get_promotion_piece() {
  if (!is_promotion()) return 0;
  int specials = get_flags() & 0b0011;
  if (specials == 0) return 2; // knight
  if (specials == 1) return 3; // bishop
  if (specials == 2) return 4; // rook
  return 5; // queen
}