#include "board.hpp"

Board::Board() {;
  for (int i = 0; i < 16; i++) pieceBB[nWhite] |= 1ull << i;
  for (int i = 48; i < 64; i++) pieceBB[nBlack] |= 1ull << i;
  for (int i = 8; i < 16; i++) pieceBB[nPawn] |= 1ull << i, pieceBB[nPawn] |= 1ull << (i + 40);
  pieceBB[nKnight] = 1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
  pieceBB[nBishop] = 1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
  pieceBB[nRook] = 1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
  pieceBB[nQueen] = 1ull << 3 | 1ull << 59;
  pieceBB[nKing] = 1ull << 4 | 1ull << 60;

  moves = std::vector<u_int16_t>();
  board_history = std::vector<u_int64_t *>();
}

bool Board::debug_print(u_int16_t move) { std::cerr << to_string() << turn << " " << move_to_string(move) << std::endl; return false; }
bool Board::debug_print() { std::cerr << to_string() << turn << std::endl; return false; }

// assumes move is valid
void Board::make_move(u_int16_t move) {
  u_int64_t *temp = new u_int64_t[8];
  std::copy(pieceBB, pieceBB + 8, temp);
  board_history.emplace_back(temp);

  int from = get_move_from(move);
  int to = get_move_to(move);
  int flags = get_move_flags(move);

  if (get_piece(from) == KING) can_castle[!turn * 2] = can_castle[!turn * 2 + 1] = false;

#ifdef DEBUG
  assert(get_piece(from) != EMPTY);
#endif

  if (((from == 0 || to == 0) && turn) || ((from == 56 || to == 56) && !turn)) can_castle[!turn * 2 + 1] = false;
  if (((from == 7 || to == 7) && turn) || ((from == 63 || to == 63) && !turn)) can_castle[!turn * 2] = false;

  bool is_capture = is_move_capture(move);

  if (flags == 0b0000) { // quiet move 
#ifdef DEBUG
    assert(is_empty(to));
#endif
    make_move_bb(from, to, is_capture);
  } else if (flags == 0b0001) { // double pawn push
#ifdef DEBUG
    assert(is_empty(to));
    // assert(board[to + (turn ? -8 : 8)] == 0 || debug_print(move));
    // assert(abs(board[from]) == 1);
#endif
    make_move_bb(from, to, is_capture);
  } else if (flags == 0b0010) { // king's side castle
    make_move_bb(from, to, is_capture);
    make_move_bb(to + 1, to - 1, is_capture);
  } else if (flags == 0b0011) { // queen's side castle
    make_move_bb(from, to, is_capture);
    make_move_bb(to - 2, to + 1, is_capture);
  } else if (flags == 0b0100) { // captures
#ifdef DEBUG
    assert(get_colour(to) != turn);
#endif
    make_move_bb(from, to, is_capture);
  } else if (flags == 0b0101) { // en passant
    make_move_bb(from, to, is_capture);
    clear(to + (turn ? -8 : 8)); // remove the pawn according to who's turn it is
  } else { // promotion
#ifdef DEBUG
    if (is_move_capture(move)) assert(get_colour(to) != turn);
    else assert(is_empty(to));
    assert(is_move_promotion(move));
#endif
    set_piece(to, get_move_promotion_piece(move), turn);
    clear(from);
  }
  moves.emplace_back(move); // backlog
  turn = !turn; // switch turns
}

void Board::undo_move() {
  u_int64_t *last_board = board_history.back();
  std::copy(last_board, last_board + 8, pieceBB);
  board_history.pop_back();
  moves.pop_back();
  turn = !turn;
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