#include <iostream>
#include <random>
#include "search.h"

typedef std::mt19937 MyRNG;

int32_t main() {
  Board board;
  Search search(board);
  std::cout << board.to_string() << '\n';
  int val = search.alpha_beta(-INT_MAX, INT_MAX, 15);
  std::cout << val << '\n';
  auto pv = search.get_principle_variation();
  reverse(pv.begin(), pv.end());
  for (Move *move : pv) {
    std::cout << move->to_string() << '\n';
    board.make_move(move);
    std::cout << board.to_string() << '\n';
  }
}