#include "board.hpp"

const int knight_moves_d[8] = { -10, -6, -17, -15, 6, 10, 15, 17 };
const int diagonal_moves_d[4] = { 7, 9, -7, -9 };
const int cardinal_moves_d[4] = { 8, 1, -8, -1 };
const u_int64_t knight_masks[8] = {
  0xfcfcfcfcfcfcfc00,
  0x3f3f3f3f3f3f3f00,
  0xfefefefefefe0000,
  0x7f7f7f7f7f7f0000,
  0x00fcfcfcfcfcfcfc,
  0x003f3f3f3f3f3f3f,
  0x0000fefefefefefe,
  0x00007f7f7f7f7f7f,
};
const u_int64_t diagonal_masks[4] = {
  0x7f7f7f7f7f7f7f00,
  0xfefefefefefefe00,
  0x00fefefefefefefe,
  0x007f7f7f7f7f7f7f,
};
const u_int64_t cardinal_masks[4] = {
  0xffffffffffffff00,
  0xfefefefefefefefe,
  0x00ffffffffffffff,
  0x7f7f7f7f7f7f7f7f,
};

const u_int8_t bit_scan_index64[64] = {
  0, 47,  1, 56, 48, 27,  2, 60,
  57, 49, 41, 37, 28, 16,  3, 61,
  54, 58, 35, 52, 50, 42, 21, 44,
  38, 32, 29, 23, 17, 11,  4, 62,
  46, 55, 26, 59, 40, 36, 15, 53,
  34, 51, 20, 43, 31, 22, 10, 45,
  25, 39, 14, 33, 19, 30,  9, 24,
  13, 18,  8, 12,  7,  6,  5, 63
};
const u_int64_t debruijn64 = 0x03f79d71b4cb0a89;

int bit_scan_forward(u_int64_t bb) {
#ifdef DEBUG
  assert(bb != 0);
#endif
  return bit_scan_index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

std::vector<u_int16_t> Board::get_moves() {
  std::vector<u_int16_t> ret;

  u_int16_t last_move = moves.empty() ? 0 : moves.back();
  int mul = turn == WHITE ? 1 : -1;

  u_int64_t white_pieces = get_white_pieces();
  u_int64_t black_pieces = get_black_pieces();
  u_int64_t occupied = white_pieces | black_pieces;
  u_int64_t empty = ~occupied;
  u_int64_t other_pieces = turn == WHITE ? black_pieces : white_pieces;

  u_int64_t pawns, pawn_push_one, pawn_push_two, pawn_capture_left, pawn_capture_right, pawn_push_one_promotion, pawn_capture_left_promotion, pawn_capture_right_promotion;
  // pawns
  if (turn == WHITE) {
    pawns = get_white_pawns();
    pawn_push_one = (pawns << 8) & empty;
    pawn_push_two = ((pawn_push_one & (0xffull << 16)) << 8) & empty;
    pawn_capture_left = ((pawns & 0xfefefefefefefefe) << 7) & other_pieces;
    pawn_capture_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << 9) & other_pieces;
    pawn_push_one_promotion = pawn_push_one & (0xffull << 56);
    pawn_capture_left_promotion = pawn_capture_left & (0xffull << 56);
    pawn_capture_right_promotion = pawn_capture_right & (0xffull << 56);
  } else {
    pawns = get_black_pawns();
    pawn_push_one = (pawns >> 8) & empty;
    pawn_push_two = ((pawn_push_one & (0xffull << 40)) >> 8) & empty;
    pawn_capture_left = ((pawns & 0x7f7f7f7f7f7f7f7f) >> 7) & other_pieces;
    pawn_capture_right = ((pawns & 0xfefefefefefefefe) >> 9) & other_pieces;
    pawn_push_one_promotion = pawn_push_one & 0xff;
    pawn_capture_left_promotion = pawn_capture_left & 0xff;
    pawn_capture_right_promotion = pawn_capture_right & 0xff;
  }
  if (last_move != 0 && get_move_flags(last_move) == 0b0001) {
    int to = get_move_to(last_move);
    if (pawns & (1ull << (to + 1)) & 0xfefefefefefefefe) {
      ret.emplace_back(create_move(to + 1, to + 8 * mul, 0b0101));
    }
    if (pawns & (1ull << (to - 1)) & 0x7f7f7f7f7f7f7f7f) {
      ret.emplace_back(create_move(to - 1, to + 8 * mul, 0b0101));
    }
  }
  for (int i = 0; i < 64; i++) {
    u_int64_t bit = 1ull << i;
    if (pawn_push_one_promotion & bit) {
      ret.emplace_back(create_move(i - 8 * mul, i, 0b1000));
      ret.emplace_back(create_move(i - 8 * mul, i, 0b1001));
      ret.emplace_back(create_move(i - 8 * mul, i, 0b1010));
      ret.emplace_back(create_move(i - 8 * mul, i, 0b1011));
    } else if (pawn_push_one & bit) {
      ret.emplace_back(create_move(i - 8 * mul, i, 0b0000));
    }

    if (pawn_push_two & bit) {
      ret.emplace_back(create_move(i - 16 * mul, i, 0b0001));
    }

    if (pawn_capture_left_promotion & bit) {
      ret.emplace_back(create_move(i - 7 * mul, i, 0b1100));
      ret.emplace_back(create_move(i - 7 * mul, i, 0b1101));
      ret.emplace_back(create_move(i - 7 * mul, i, 0b1110));
      ret.emplace_back(create_move(i - 7 * mul, i, 0b1111));
    } else if (pawn_capture_left & bit) {
      ret.emplace_back(create_move(i - 7 * mul, i, 0b0100));
    }

    if (pawn_capture_right_promotion & bit) {
      ret.emplace_back(create_move(i - 9 * mul, i, 0b1100));
      ret.emplace_back(create_move(i - 9 * mul, i, 0b1101));
      ret.emplace_back(create_move(i - 9 * mul, i, 0b1110));
      ret.emplace_back(create_move(i - 9 * mul, i, 0b1111));
    } else if (pawn_capture_right & bit) {
      ret.emplace_back(create_move(i - 9 * mul, i, 0b0100));
    }
  }

  // knight
  u_int64_t knights = turn == WHITE ? get_white_knights() : get_black_knights();
  for (int d = 0; d < 8; d++) {
    u_int64_t temp = knights & knight_masks[d];
    if (d < 4) temp >>= -knight_moves_d[d];
    else temp <<= knight_moves_d[d];
    u_int64_t temp_captures = temp & other_pieces;
    temp = (temp & empty) | temp_captures;
    while (temp) {
      u_int64_t ls1b = temp & -temp;
      int i = bit_scan_forward(ls1b);
      ret.emplace_back(create_move(i - knight_moves_d[d], i, bool(temp_captures & ls1b) << 2));
      temp ^= ls1b;
    }
  }

  // bishop and queen
  u_int64_t bishops = turn == WHITE ? get_white_bishops() : get_black_bishops();
  u_int64_t queens = turn == WHITE ? get_white_queens() : get_black_queens();
  for (int d = 0; d < 4; d++) {
    u_int64_t temp_bishops = bishops;
    u_int64_t temp_queens = queens;
    for (int m = 0; temp_bishops || temp_queens; m++) {
      if (d < 2) temp_bishops <<= diagonal_moves_d[d], temp_queens <<= diagonal_moves_d[d];
      else temp_bishops >>= -diagonal_moves_d[d], temp_queens >>= -diagonal_moves_d[d];
      temp_bishops &= diagonal_masks[d]; temp_queens &= diagonal_masks[d];
      u_int64_t temp_bishops_captures = temp_bishops & other_pieces;
      u_int64_t temp_queens_captures = temp_queens & other_pieces;
      temp_bishops = (temp_bishops & empty) | temp_bishops_captures;
      temp_queens = (temp_queens & empty) | temp_queens_captures;
      u_int64_t temp = temp_bishops;
      while (temp) {
        u_int64_t ls1b = temp & -temp;
        int i = bit_scan_forward(ls1b);
        ret.emplace_back(create_move(i - (m + 1) * diagonal_moves_d[d], i, bool(temp_bishops_captures & ls1b) << 2));
        temp ^= ls1b;
      }
      temp = temp_queens;
      while (temp) {
        u_int64_t ls1b = temp & -temp;
        int i = bit_scan_forward(ls1b);
        ret.emplace_back(create_move(i - (m + 1) * diagonal_moves_d[d], i, bool(temp_queens_captures & ls1b) << 2));
        temp ^= ls1b;
      }
    }
  }

  // rook and queen
  u_int64_t rooks = turn == WHITE ? get_white_rooks() : get_black_rooks();
  for (int d = 0; d < 4; d++) {
    int cc = cardinal_moves_d[d];
    u_int64_t temp_rooks = rooks;
    u_int64_t temp_queens = queens;
    for (int m = 0; temp_rooks || temp_queens; m++) {
      if (d < 2) temp_rooks <<= cc, temp_queens <<= cc;
      else temp_rooks >>= -cc, temp_queens >>= -cc;
      temp_rooks &= cardinal_masks[d]; temp_queens &= cardinal_masks[d];
      u_int64_t temp_rooks_captures = temp_rooks & other_pieces;
      u_int64_t temp_queens_captures = temp_queens & other_pieces;
      temp_rooks = (temp_rooks & empty) | temp_rooks_captures;
      temp_queens = (temp_queens & empty) | temp_queens_captures;
      u_int64_t temp = temp_rooks;
      while (temp) {
        u_int64_t ls1b = temp & -temp;
        int i = bit_scan_forward(ls1b);
        ret.emplace_back(create_move(i - (m + 1) * cc, i, bool(temp_rooks_captures & ls1b) << 2));
        temp ^= ls1b;
      }
      temp = temp_queens;
      while (temp) {
        u_int64_t ls1b = temp & -temp;
        int i = bit_scan_forward(ls1b);
        ret.emplace_back(create_move(i - (m + 1) * cc, i, bool(temp_queens_captures & ls1b) << 2));
        temp ^= ls1b;
      }
    }
  }

  // king
  u_int64_t king = turn == WHITE ? get_white_king() : get_black_king();
  for (int d = 0; d < 4; d++) {
    int cc = cardinal_moves_d[d], dd = diagonal_moves_d[d];
    u_int64_t temp_king_cardinal = king;
    u_int64_t temp_king_diagonal = king;
    if (d < 2) temp_king_cardinal <<= cc, temp_king_diagonal <<= dd;
    else temp_king_cardinal >>= -cc, temp_king_diagonal >>= -dd;
    temp_king_cardinal &= cardinal_masks[d];
    temp_king_diagonal &= diagonal_masks[d];
    u_int64_t temp_king_cardinal_captures = temp_king_cardinal & other_pieces;
    u_int64_t temp_king_diagonal_captures = temp_king_diagonal & other_pieces;
    temp_king_cardinal = (temp_king_cardinal & empty) | temp_king_cardinal_captures;
    temp_king_diagonal = (temp_king_diagonal & empty) | temp_king_diagonal_captures;
    if (temp_king_cardinal) {
#ifdef DEBUG
      assert(temp_king_cardinal == (temp_king_cardinal & -temp_king_cardinal));
#endif
      int i = bit_scan_forward(temp_king_cardinal);
      ret.emplace_back(create_move(i - cc, i, bool(temp_king_cardinal & temp_king_diagonal_captures) << 2));
    }
    if (temp_king_diagonal) {
#ifdef DEBUG
      assert(temp_king_diagonal == (temp_king_diagonal & -temp_king_diagonal));
#endif
      int i = bit_scan_forward(temp_king_diagonal);
      ret.emplace_back(create_move(i - dd, i, bool(temp_king_diagonal & temp_king_cardinal_captures) << 2));
    }
  }

  return ret;
}