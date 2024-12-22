#include <iostream>
#include <cassert>
#include "board.h"

Board::Board() {
  for (int i = 8; i < 16; i++) board[i] = 1;
  board[0] = board[7] = 4; // rook
  board[1] = board[6] = 2; // knight
  board[2] = board[5] = 3; // bishop
  board[3] = 5; // queen
  board[4] = 6; // king

  for (int i = 48; i < 56; i++) board[i] = -1;
  board[56] = board[63] = -4;
  board[57] = board[62] = -2;
  board[58] = board[61] = -3;
  board[59] = -5;
  board[60] = -6;

  moves = std::vector<Move *>();
  board_history = std::vector<int8_t *>();
}

int Board::evaluate() {
  int score = 0;
  for (int i = 0; i < 64; i++) {
    int piece = board[i];
    int piece_colour = piece > 0 ? 1 : -1;
    piece = abs(piece);
    if (piece == 0) continue;
    switch (piece) {
      case 1: score += piece_colour * 100; break;
      case 2: score += piece_colour * 320; break;
      case 3: score += piece_colour * 330; break;
      case 4: score += piece_colour * 500; break;
      case 5: score += piece_colour * 900; break;
      case 6: score += piece_colour * 20000; break;
    }
  }
  return score * (turn ? 1 : -1);
}

bool Board::print(Move *move) {
  std::cerr << to_string() << turn << " " << move->to_string() << std::endl;
  return false;
}

// assumes move is valid
void Board::make_move(Move *move, bool search_flag) {
  if (search_flag) {
    int8_t *temp = new int8_t[64];
    std::copy(board, board + 64, temp);
    board_history.emplace_back(temp);
  }
  int from = move->get_from();
  int to = move->get_to();
  int flags = move->get_flags();
  if (abs(board[from]) == 6) {
    can_castle[!turn * 2] = can_castle[!turn * 2 + 1] = false;
  }
  assert(board[from] != 0 || print(move));
  if (((from == 0 || to == 0) && turn) || ((from == 56 || to == 56) && !turn)) can_castle[!turn * 2 + 1] = false;
  if (((from == 7 || to == 7) && turn) || ((from == 63 || to == 63) && !turn)) can_castle[!turn * 2] = false;
  if (flags == 0b0000) { // quiet move
    assert(board[to] == 0 || print(move));
    board[to] = board[from];
    board[from] = 0;
  } else if (flags == 0b0001) { // double pawn push
    assert(board[to] == 0);
    assert(board[to + (turn ? -8 : 8)] == 0 || print(move));
    assert(abs(board[from]) == 1);
    board[to] = board[from];
    board[from] = 0;
  } else if (flags == 0b0010) { // king's side castle
    board[to] = board[from];
    board[to - 1] = board[to + 1];
    board[from] = 0;
    board[to + 1] = 0;
  } else if (flags == 0b0011) { // queen's side castle
    board[to] = board[from];
    board[to + 1] = board[to - 2];
    board[from] = 0;
    board[to - 2] = 0;
  } else if (flags == 0b0100) { // captures
    assert(board[to] > 0 != turn);
    board[to] = board[from];
    board[from] = 0;
  } else if (flags == 0b0101) { // en passant
    board[to] = board[from];
    board[from] = 0;
    board[to + (turn ? -8 : 8)] = 0; // remove the pawn according to who's turn it is
  } else { // promotion
    if (move->is_capture()) assert(board[to] > 0 != turn);
    else assert(board[to] == 0);
    board[to] = move->get_promotion_piece();
    board[from] = 0;
  }
  moves.emplace_back(move); // backlog
  turn ^= true; // switch turns
}

void Board::undo_move() {
  int8_t *last_board = board_history.back();
  std::copy(last_board, last_board + 64, board);
  board_history.pop_back();
  moves.pop_back();
  turn ^= true;
}

std::vector<Move *> Board::get_moves() {
  std::vector<Move *> ret;
  for (int i = 0; i < 64; i++) {
    int8_t piece = abs(board[i]);
    if (board[i] == 0 || (board[i] > 0 != turn)) continue;
    if (piece == 1) { // pawn
      if (turn && i < 16 && board[i + 8] == 0 && board[i + 16] == 0) { // double pawn moves
        // if (i == 13) std::cerr << to_string() << std::endl;
        ret.emplace_back(new Move(i, i + 16, 0b0001));
      }
      if (!turn && i >= 48 && board[i - 8] == 0 && board[i - 16] == 0) { // double pawn moves
        ret.emplace_back(new Move(i, i - 16, 0b0001));
      }

      if (turn && i < 48 && board[i + 8] == 0) { // quiet pawn push
        ret.emplace_back(new Move(i, i + 8, 0b0000));
      }
      if (!turn && i >= 16 && board[i - 8] == 0) { // quiet pawn push
        ret.emplace_back(new Move(i, i - 8, 0b0000));
      }

      if (turn && i >= 48 && board[i + 8] == 0) { // pawn push promotion
        ret.emplace_back(new Move(i, i + 8, 0b1000));
        ret.emplace_back(new Move(i, i + 8, 0b1001));
        ret.emplace_back(new Move(i, i + 8, 0b1010));
        ret.emplace_back(new Move(i, i + 8, 0b1011));
      }
      if (!turn && i < 16 && board[i - 8] == 0) { // pawn push promotion
        ret.emplace_back(new Move(i, i - 8, 0b1000));
        ret.emplace_back(new Move(i, i - 8, 0b1001));
        ret.emplace_back(new Move(i, i - 8, 0b1010));
        ret.emplace_back(new Move(i, i - 8, 0b1011));
      }

      if (turn && i % 8 != 0 && board[i + 7] < 0) { // capture to the left
        if (i >= 48) {
          ret.emplace_back(new Move(i, i + 7, 0b1100));
          ret.emplace_back(new Move(i, i + 7, 0b1101));
          ret.emplace_back(new Move(i, i + 7, 0b1110));
          ret.emplace_back(new Move(i, i + 7, 0b1111));
        } else {
          ret.emplace_back(new Move(i, i + 7, 0b0100));
        }
      }
      if (turn && i % 8 != 7 && board[i + 9] < 0) { // capture to the right
        if (i >= 48) {
          ret.emplace_back(new Move(i, i + 9, 0b1100));
          ret.emplace_back(new Move(i, i + 9, 0b1101));
          ret.emplace_back(new Move(i, i + 9, 0b1110));
          ret.emplace_back(new Move(i, i + 9, 0b1111));
        } else {
          ret.emplace_back(new Move(i, i + 9, 0b0100));
        }
      }
      if (!turn && i % 8 != 0 && board[i - 9] > 0) { // right (in the eyes of black)
        if (i < 16) {
          ret.emplace_back(new Move(i, i - 9, 0b1100));
          ret.emplace_back(new Move(i, i - 9, 0b1101));
          ret.emplace_back(new Move(i, i - 9, 0b1110));
          ret.emplace_back(new Move(i, i - 9, 0b1111));
        } else {
          ret.emplace_back(new Move(i, i - 9, 0b0100));
        }
      }
      if (!turn && i % 8 != 7 && board[i - 7] > 0) { // left (in the eyes of black)
        if (i < 16) {
          ret.emplace_back(new Move(i, i - 7, 0b1100));
          ret.emplace_back(new Move(i, i - 7, 0b1101));
          ret.emplace_back(new Move(i, i - 7, 0b1110));
          ret.emplace_back(new Move(i, i - 7, 0b1111));
        } else {
          ret.emplace_back(new Move(i, i - 7, 0b0100));
        }
      }
      // en passant:
      if (moves.size() == 0) continue;
      Move *last_move = moves.back();
      if (i % 8 != 0 && last_move->get_to() == i - 1 && last_move->get_flags() == 0b0001) { // left (eyes of white)
        ret.emplace_back(new Move(i, turn ? i + 7 : i - 9, 0b0101));
      }
      if (i % 8 != 7 && last_move->get_to() == i + 1 && last_move->get_flags() == 0b0001) { // right (eyes of white)
        ret.emplace_back(new Move(i, turn ? i + 9 : i - 7, 0b0101));
      }
    } else if (piece == 2) { // knight
      if (i % 8 >= 2 && i >= 8 && (board[i - 10] == 0 || (board[i - 10] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 10, board[i - 10] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 <= 5 && i >= 8 && (board[i - 6] == 0 || (board[i - 6] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 6, board[i - 6] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 >= 1 && i >= 16 && (board[i - 17] == 0 || (board[i - 17] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 17, board[i - 17] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 <= 6 && i >= 16 && (board[i - 15] == 0 || (board[i - 15] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 15, board[i - 15] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 >= 2 && i < 56 && (board[i + 6] == 0 || (board[i + 6] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 6, board[i + 6] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 <= 5 && i < 56 && (board[i + 10] == 0 || (board[i + 10] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 10, board[i + 10] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 >= 1 && i < 48 && (board[i + 15] == 0 || (board[i + 15] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 15, board[i + 15] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 <= 6 && i < 48 && (board[i + 17] == 0 || (board[i + 17] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 17, board[i + 17] == 0 ? 0b0000 : 0b0100));
      }
    }
    if (piece == 3 || piece == 5) { // bishop or queen
      if (i % 8 != 0 && i < 56) for (int j = i + 7; j % 8 != 7 && j < 64; j += 7) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      if (i % 8 != 7 && i < 56) for (int j = i + 9; j % 8 != 0 && j < 64; j += 9) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      if (i % 8 != 7 && i >= 8) for (int j = i - 7; j % 8 != 0 && j >= 0; j -= 7) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      if (i % 8 != 0 && i >= 8) for (int j = i - 9; j % 8 != 7 && j >= 0; j -= 9) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
    }
    if (piece == 4 || piece == 5) { // rook or queen
      for (int j = i + 8; j < 64; j += 8) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      for (int j = i - 8; j >= 0; j -= 8) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      if (i % 8 != 7) for (int j = i + 1; j % 8 != 0 && j < 64; j++) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
      if (i % 8 != 0) for (int j = i - 1; j % 8 != 7 && j >= 0; j--) {
        if (board[j] == 0) {
          ret.emplace_back(new Move(i, j, 0b0000));
        } else {
          if (board[j] > 0 != turn) {
            ret.emplace_back(new Move(i, j, 0b0100));
          }
          break;
        }
      }
    }
    if (piece == 6) {
      if (i % 8 != 0 && i < 56 && (board[i + 7] == 0 || (board[i + 7] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 7, board[i + 7] == 0 ? 0b0000 : 0b0100));
      }
      if (i < 56 && (board[i + 8] == 0 || (board[i + 8] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 8, board[i + 8] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 != 7 && i < 56 && (board[i + 9] == 0 || (board[i + 9] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 9, board[i + 9] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 != 0 && i >= 8 && (board[i - 9] == 0 || (board[i - 9] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 9, board[i - 9] == 0 ? 0b0000 : 0b0100));
      }
      if (i >= 8 && (board[i - 8] == 0 || (board[i - 8] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 8, board[i - 8] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 != 7 && i >= 8 && (board[i - 7] == 0 || (board[i - 7] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 7, board[i - 7] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 != 0 && (board[i - 1] == 0 || (board[i - 1] > 0 != turn))) {
        ret.emplace_back(new Move(i, i - 1, board[i - 1] == 0 ? 0b0000 : 0b0100));
      }
      if (i % 8 != 7 && (board[i + 1] == 0 || (board[i + 1] > 0 != turn))) {
        ret.emplace_back(new Move(i, i + 1, board[i + 1] == 0 ? 0b0000 : 0b0100));
      }
      if (can_castle[!turn * 2] && ((turn && i == 4) || (!turn && i == 60)) &&
          board[i + 1] == 0 && board[i + 2] == 0 && abs(board[i + 3]) == 4) {
        ret.emplace_back(new Move(i, i + 2, 0b0010));
      }
      if (can_castle[!turn * 2 + 1] && ((turn && i == 4) || (!turn && i == 60)) &&
          board[i - 1] == 0 && board[i - 2] == 0 && board[i - 3] == 0 && abs(board[i - 4]) == 4) {
        ret.emplace_back(new Move(i, i - 2, 0b0011));
      }
    }
  }
  return ret;
}

std::string Board::to_string(int8_t *temp_board) {
  if (temp_board == nullptr) temp_board = board;
  std::vector<std::string> result;
  std::string temp = "";
  for (int i = 0; i < 64; i++) {
    if (i != 0 && i % 8 == 0) result.emplace_back(temp), temp = "";
    std::string c;
    switch (temp_board[i]) {
      case 0: c = "."; break;
      case 1: c = "P"; break;
      case 2: c = "N"; break;
      case 3: c = "B"; break;
      case 4: c = "R"; break;
      case 5: c = "Q"; break;
      case 6: c = "K"; break;
      case -1: c = "p"; break;
      case -2: c = "n"; break;
      case -3: c = "b"; break;
      case -4: c = "r"; break;
      case -5: c = "q"; break;
      case -6: c = "k"; break;
    }
    temp += c + " ";
  }
  result.emplace_back(temp), temp = "";
  reverse(result.begin(), result.end());
  for (std::string i : result) temp += i + "\n";
  return temp;
}