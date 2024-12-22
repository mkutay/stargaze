#include <string>
#include <vector>
#include "move.h"

class Board {
private:
  int8_t board[64] = { 0 };
  std::vector<int8_t *> board_history;
  bool turn = true; // true: white, false: black
  bool can_castle[4] = {true}; // 0: white king's side, 1: white queen's side, 2: black king's side, 3: black queen's side
  std::vector<Move *> moves;
public:
  Board();
  void make_move(Move *move, bool search_flag = false);
  void undo_move();
  std::vector<Move *> get_moves();
  std::string to_string(int8_t *temp_board = nullptr);
  int evaluate();
  bool get_turn() { return turn; }
  bool print(Move *move);
};