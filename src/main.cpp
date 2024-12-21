#include <iostream>
#include <random>
#include "search.h"

typedef std::mt19937 MyRNG;

int32_t main() {
  Board board;
  Search search(board);
  for (int i = 0; i < 5; i++) {
    std::cout << board.to_string() << '\n';
    int val = board.get_turn() ? search.alpha_beta(-INT_MAX, INT_MAX, 15) : -search.alpha_beta(-INT_MAX, INT_MAX, 15);
    std::cout << val << '\n';
    Move *move = search.get_principle_variation()->back();
    std::cout << move->to_string() << '\n';
    board.make_move(move);
    search.get_principle_variation()->clear();
  }
}