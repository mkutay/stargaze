#include "board.h"

Board::Board() {;
  for (int i = 0; i < 16; i++) pieceBB[nWhite] |= 1ull << i;
  for (int i = 48; i < 64; i++) pieceBB[nBlack] |= 1ull << i;
  for (int i = 8; i < 16; i++) pieceBB[nPawn] |= 1ull << i, pieceBB[nPawn] |= 1ull << (i + 40);
  pieceBB[nKnight] = 1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
  pieceBB[nBishop] = 1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
  pieceBB[nRook] = 1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
  pieceBB[nQueen] = 1ull << 3 | 1ull << 59;
  pieceBB[nKing] = 1ull << 4 | 1ull << 60;

  moves = std::vector<Move *>();
  board_history = std::vector<u_int64_t *>();
}

int Board::evaluate() {
  int score = 0;
  for (int i = 0; i < 64; i++) {
    Piece p = get_piece(i);
    int colour_mul = get_colour(i) == WHITE ? 1 : -1;
    switch (p) {
      case EMPTY: break;
      case W_PAWN: score += 100; break;
      case B_PAWN: score += -100; break;
      case KNIGHT: score += colour_mul * 320; break;
      case BISHOP: score += colour_mul * 330; break;
      case ROOK: score += colour_mul * 500; break;
      case QUEEN: score += colour_mul * 900; break;
      case KING: score += colour_mul * 20000; break;
    }
  }
  return score * (turn == WHITE ? 1 : -1);
}

bool Board::debug_print(Move *move) {
  std::cerr << to_string() << turn << " " << move->to_string() << std::endl;
  return false;
}

// assumes move is valid
void Board::make_move(Move *move) {
  u_int64_t *temp = new u_int64_t[8];
  std::copy(pieceBB, pieceBB + 8, temp);
  board_history.emplace_back(temp);

  int from = move->get_from();
  int to = move->get_to();
  int flags = move->get_flags();
  bool is_capture = move->is_capture();
  if (get_piece(from) == KING) {
    can_castle[!turn * 2] = can_castle[!turn * 2 + 1] = false;
  }
  assert(get_piece(from) != EMPTY);
  if (((from == 0 || to == 0) && turn) || ((from == 56 || to == 56) && !turn)) can_castle[!turn * 2 + 1] = false;
  if (((from == 7 || to == 7) && turn) || ((from == 63 || to == 63) && !turn)) can_castle[!turn * 2] = false;
  if (flags == 0b0000) { // quiet move
    assert(is_empty(to));
    copy(to, from, is_capture, turn);
  } else if (flags == 0b0001) { // double pawn push
    assert(is_empty(to));
    // assert(board[to + (turn ? -8 : 8)] == 0 || debug_print(move));
    // assert(abs(board[from]) == 1);
    copy(to, from, is_capture, turn); 
  } else if (flags == 0b0010) { // king's side castle
    copy(to, from, is_capture, turn);
    copy(to - 1, to + 1, is_capture, turn);
  } else if (flags == 0b0011) { // queen's side castle
    copy(to, from, is_capture, turn);
    copy(to + 1, to - 2, is_capture, turn);
  } else if (flags == 0b0100) { // captures
    assert(get_colour(to) != turn);
    copy(to, from, is_capture, turn);
  } else if (flags == 0b0101) { // en passant
    copy(to, from, is_capture, turn);
    clear(to + (turn ? -8 : 8)); // remove the pawn according to who's turn it is
  } else { // promotion
    if (move->is_capture()) assert(get_colour(to) != turn);
    else assert(is_empty(to));
    assert(move->is_promotion());
    set_piece(to, move->get_promotion_piece(), turn);
    clear(from);
  }
  moves.emplace_back(move); // backlog
  turn = turn == WHITE ? BLACK : WHITE; // switch turns
}

void Board::undo_move() {
  u_int64_t *last_board = board_history.back();
  std::copy(last_board, last_board + 8, pieceBB);
  board_history.pop_back();
  moves.pop_back();
  turn = turn == WHITE ? BLACK : WHITE;
}

std::vector<Move *> Board::get_moves() {
  std::vector<Move *> ret;

  Move *last_move = moves.empty() ? nullptr : moves.back();
  int mul = turn == WHITE ? 1 : -1;

  u_int64_t white_pieces = pieceBB[nWhite];
  u_int64_t black_pieces = pieceBB[nBlack];
  u_int64_t occupied = white_pieces | black_pieces;
  u_int64_t empty = ~occupied;

  u_int64_t pawns, pawn_push_one, pawn_push_two, pawn_capture_left, pawn_capture_right, pawn_push_one_promotion, pawn_capture_left_promotion, pawn_capture_right_promotion;
  // pawns
  if (turn == WHITE) {
    pawns = get_white_pawns();
    pawn_push_one = (pawns << 8) & empty;
    pawn_push_two = ((pawn_push_one & (0xffull << 16)) << 8) & empty;
    pawn_capture_left = ((pawns & 0xfefefefefefefefe) << 7) & black_pieces;
    pawn_capture_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << 9) & black_pieces;
    pawn_push_one_promotion = pawn_push_one & 0xff00000000000000;
    pawn_capture_left_promotion = pawn_capture_left & 0xff00000000000000;
    pawn_capture_right_promotion = pawn_capture_right & 0xff00000000000000;
  } else {
    pawns = get_black_pawns();
    pawn_push_one = (pawns >> 8) & empty;
    pawn_push_two = ((pawn_push_one & (0xffull << 40)) >> 8) & empty;
    pawn_capture_left = ((pawns & 0x7f7f7f7f7f7f7f7f) >> 7) & white_pieces;
    pawn_capture_right = ((pawns & 0xfefefefefefefefe) >> 9) & white_pieces;
    pawn_push_one_promotion = pawn_push_one & 0xff;
    pawn_capture_left_promotion = pawn_capture_left & 0xff;
    pawn_capture_right_promotion = pawn_capture_right & 0xff;
  }
  if (last_move != nullptr && last_move->get_flags() == 0b0001) {
    int to = last_move->get_to();
    if ((pawns & (1ull << (to + 1))) && to % 8 != 0) {
      ret.emplace_back(new Move(to + 1, to + 8 * mul, 0b0101));
    }
    if ((pawns & (1ull << (to - 1))) && to % 8 != 7) {
      ret.emplace_back(new Move(to - 1, to + 8 * mul, 0b0101));
    }
  }
  for (int i = 0; i < 64; i++) {
    u_int64_t bit = 1ull << i;
    if (pawn_push_one_promotion & bit) {
      ret.emplace_back(new Move(i - 8 * mul, i, 0b1000));
      ret.emplace_back(new Move(i - 8 * mul, i, 0b1001));
      ret.emplace_back(new Move(i - 8 * mul, i, 0b1010));
      ret.emplace_back(new Move(i - 8 * mul, i, 0b1011));
    } else if (pawn_push_one & bit) {
      ret.emplace_back(new Move(i - 8 * mul, i, 0b0000));
    }

    if (pawn_push_two & bit) {
      ret.emplace_back(new Move(i - 16 * mul, i, 0b0001));
    }

    if (pawn_capture_left_promotion & bit) {
      ret.emplace_back(new Move(i - 7 * mul, i, 0b1100));
      ret.emplace_back(new Move(i - 7 * mul, i, 0b1101));
      ret.emplace_back(new Move(i - 7 * mul, i, 0b1110));
      ret.emplace_back(new Move(i - 7 * mul, i, 0b1111));
    } else if (pawn_capture_left & bit) {
      ret.emplace_back(new Move(i - 7 * mul, i, 0b0100));
    }

    if (pawn_capture_right_promotion & bit) {
      ret.emplace_back(new Move(i - 9 * mul, i, 0b1100));
      ret.emplace_back(new Move(i - 9 * mul, i, 0b1101));
      ret.emplace_back(new Move(i - 9 * mul, i, 0b1110));
      ret.emplace_back(new Move(i - 9 * mul, i, 0b1111));
    } else if (pawn_capture_right & bit) {
      ret.emplace_back(new Move(i - 9 * mul, i, 0b0100));
    }
  }

  // knight
  u_int64_t knights = turn == WHITE ? get_white_knights() : get_black_knights();
  u_int64_t knight_masks[8] = {
    0xfcfcfcfcfcfcfc00,
    0x3f3f3f3f3f3f3f00,
    0xfefefefefefe0000,
    0x7f7f7f7f7f7f0000,
    0x00fcfcfcfcfcfcfc,
    0x003f3f3f3f3f3f3f,
    0x0000fefefefefefe,
    0x00007f7f7f7f7f7f,
  };
  int knight_moves_d[8] = { -10, -6, -17, -15, 6, 10, 15, 17 };
  for (int d = 0; d < 8; d++) {
    u_int64_t temp_knights = knights;
    temp_knights &= knight_masks[d];
    if (d < 4) temp_knights >>= -knight_moves_d[d];
    else temp_knights <<= knight_moves_d[d];
    u_int64_t temp_knights_occupied = temp_knights & occupied;
    temp_knights &= empty;
    for (int i = 0; i < 64; i++) {
      u_int64_t bit = 1ull << i;
      if (temp_knights_occupied & bit && get_colour(i) != turn) {
        ret.emplace_back(new Move(i - knight_moves_d[d], i, 0b0100));
      } else if (temp_knights & bit) {
        ret.emplace_back(new Move(i - knight_moves_d[d], i, 0b0000));
      }
    }
  }

  // bishop and queen
  u_int64_t bishops = turn == WHITE ? get_white_bishops() : get_black_bishops();
  u_int64_t queens = turn == WHITE ? get_white_queens() : get_black_queens();
  int diagonal_moves_d[4] = { 7, 9, -7, -9 };
  u_int64_t diagonal_masks[4] = {
    0x7f7f7f7f7f7f7f00,
    0xfefefefefefefe00,
    0x00fefefefefefefe,
    0x007f7f7f7f7f7f7f,
  };
  for (int d = 0; d < 4; d++) {
    u_int64_t temp_bishops = bishops;
    u_int64_t temp_queens = queens;
    for (int m = 0; m < 8; m++) {
      if (d < 2) temp_bishops <<= diagonal_moves_d[d], temp_queens <<= diagonal_moves_d[d];
      else temp_bishops >>= -diagonal_moves_d[d], temp_queens >>= -diagonal_moves_d[d];
      temp_bishops &= diagonal_masks[d];
      temp_queens &= diagonal_masks[d];
      u_int64_t temp_bishops_occupied = temp_bishops & occupied;
      u_int64_t temp_queens_occupied = temp_queens & occupied;
      temp_bishops ^= temp_bishops_occupied;
      temp_queens ^= temp_queens_occupied;
      if (temp_bishops != 0 || temp_queens != 0) for (int i = 0; i < 64; i++) {
        u_int64_t bit = 1ull << i;
        if (temp_bishops_occupied & bit && get_colour(i) != turn) {
          ret.emplace_back(new Move(i - (m + 1) * diagonal_moves_d[d], i, 0b0100));
        } else if (temp_bishops & bit) {
          ret.emplace_back(new Move(i - (m + 1) * diagonal_moves_d[d], i, 0b0000));
        }
        if (temp_queens_occupied & bit && get_colour(i) != turn) {
          ret.emplace_back(new Move(i - (m + 1) * diagonal_moves_d[d], i, 0b0100));
        } else if (temp_queens & bit) {
          ret.emplace_back(new Move(i - (m + 1) * diagonal_moves_d[d], i, 0b0000));
        }
      }
    }
  }

  // rook and queen
  u_int64_t rooks = turn == WHITE ? get_white_rooks() : get_black_rooks();
  int cardinal_moves_d[4] = { 8, 1, -8, -1 };
  u_int64_t cardinal_masks[4] = {
    0xffffffffffffff00,
    0xfefefefefefefefe,
    0x00ffffffffffffff,
    0x7f7f7f7f7f7f7f7f,
  };
  for (int d = 0; d < 4; d++) {
    u_int64_t temp_rooks = rooks;
    u_int64_t temp_queens = queens;
    for (int m = 0; m < 8; m++) {
      if (d < 2) temp_rooks <<= cardinal_moves_d[d], temp_queens <<= cardinal_moves_d[d];
      else temp_rooks >>= -cardinal_moves_d[d], temp_queens >>= -cardinal_moves_d[d];
      temp_rooks &= cardinal_masks[d];
      temp_queens &= cardinal_masks[d];
      u_int64_t temp_rooks_occupied = temp_rooks & occupied;
      u_int64_t temp_queens_occupied = temp_queens & occupied;
      temp_rooks ^= temp_rooks_occupied;
      temp_queens ^= temp_queens_occupied;
      if (temp_rooks != 0 || temp_queens != 0) for (int i = 0; i < 64; i++) {
        u_int64_t bit = 1ull << i;
        if (temp_rooks_occupied & bit && get_colour(i) != turn) {
          ret.emplace_back(new Move(i - (m + 1) * cardinal_moves_d[d], i, 0b0100));
        } else if (temp_rooks & bit) {
          ret.emplace_back(new Move(i - (m + 1) * cardinal_moves_d[d], i, 0b0000));
        }
        if (temp_queens_occupied & bit && get_colour(i) != turn) {
          ret.emplace_back(new Move(i - (m + 1) * cardinal_moves_d[d], i, 0b0100));
        } else if (temp_queens & bit) {
          ret.emplace_back(new Move(i - (m + 1) * cardinal_moves_d[d], i, 0b0000));
        }
      }
    }
  }

  // king
  u_int64_t king = turn == WHITE ? get_white_king() : get_black_king();
  for (int d = 0; d < 4; d++) {
    u_int64_t temp_king_cardinal = king;
    u_int64_t temp_king_diagonal = king;
    if (d < 2) temp_king_cardinal <<= cardinal_moves_d[d], temp_king_diagonal <<= diagonal_moves_d[d];
    else temp_king_cardinal >>= -cardinal_moves_d[d], temp_king_diagonal >>= -diagonal_moves_d[d];
    temp_king_cardinal &= cardinal_masks[d];
    temp_king_diagonal &= diagonal_masks[d];
    u_int64_t temp_king_cardinal_occupied = temp_king_cardinal & occupied;
    u_int64_t temp_king_diagonal_occupied = temp_king_diagonal & occupied;
    temp_king_cardinal &= empty;
    temp_king_diagonal &= empty;
    for (int i = 0; i < 64; i++) {
      u_int64_t bit = 1ull << i;
      if ((temp_king_cardinal_occupied & bit) && get_colour(i) != turn) {
        ret.emplace_back(new Move(i - cardinal_moves_d[d], i, 0b0100));
      } else if (temp_king_cardinal & bit) {
        ret.emplace_back(new Move(i - cardinal_moves_d[d], i, 0b0000));
      }
      if ((temp_king_diagonal_occupied & bit) && get_colour(i) != turn) {
        ret.emplace_back(new Move(i - diagonal_moves_d[d], i, 0b0100));
      } else if (temp_king_diagonal & bit) {
        ret.emplace_back(new Move(i - diagonal_moves_d[d], i, 0b0000));
      }
    }
  }

  return ret;
}

std::string Board::to_string() {
  std::vector<std::string> result;
  std::string temp = "";
  for (int i = 0; i < 64; i++) {
    if (i != 0 && i % 8 == 0) result.emplace_back(temp), temp = "";
    std::string ch;
    Piece p = get_piece(i);
    Colour c = get_colour(i);
    if (c == WHITE) {
      switch (p) {
        case W_PAWN: ch = "P"; break;
        case KNIGHT: ch = "N"; break;
        case BISHOP: ch = "B"; break;
        case ROOK: ch = "R"; break;
        case QUEEN: ch = "Q"; break;
        case KING: ch = "K"; break;
        case EMPTY: ch = "."; break;
        case B_PAWN: assert(false); break;
      }
    } else {
      switch (p) {
        case B_PAWN: ch = "p"; break;
        case KNIGHT: ch = "n"; break;
        case BISHOP: ch = "b"; break;
        case ROOK: ch = "r"; break;
        case QUEEN: ch = "q"; break;
        case KING: ch = "k"; break;
        case EMPTY: ch = "."; break;
        case W_PAWN: assert(false); break;
      }
    }
    temp += ch + " ";
  }
  result.emplace_back(temp), temp = "";
  reverse(result.begin(), result.end());
  for (std::string i : result) temp += i + "\n";
  return temp;
}