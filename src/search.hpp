#include <vector>
#include "board.hpp"

typedef struct PVLine {
  std::vector<Move *> moves;
  PVLine(int max_depth) : moves(max_depth) {}
} PVLine;

class Search {
private:
  Board *board;
  int quiescence(int alpha, int beta);
public:
  Search(Board *board);
  int alpha_beta(int alpha, int beta, int depth_left, PVLine *pline);
};