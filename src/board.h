#include <string>
#include <vector>
#include "move.h"

class Board {
private:
  int8_t board[64] = {0};
  bool turn = true;
  bool can_castle[4] = {true}; // 0: white king's side, 1: white queen's side, 2: black king's side, 3: black queen's side
  std::vector<Move *> moves;
public:
  Board();
  void make_move(Move *move);
  std::vector<Move *> get_moves();
  bool is_in_check();
  std::string to_string();
};