#include <vector>
#include "board.h"

class Search {
private:
  Board board;
  std::vector<Move *> principle_variation;
  int quisce(int alpha, int beta);
public:
  Search(Board &board) { this->board = board; }
  int alpha_beta(int alpha, int beta, int depth_left);
  std::vector<Move *> get_principle_variation() { return principle_variation; }
};