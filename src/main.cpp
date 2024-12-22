#include <iostream>
#include <random>
#include "search.h"

typedef std::mt19937 MyRNG;
int MAX_DEPTH = 7;


int32_t main() {
  Board board;
  // auto moves = board.get_moves();
  // for (auto move : moves) {
  //   std::cout << move->to_string() << '\n';
  // }
  Search search(&board);
  for (int i = 0; i < 10; i++) {
    std::cout << "\nmain: board:\n" << board.to_string();

    PVLine line(MAX_DEPTH);
    int val = search.alpha_beta(-INT_MAX, INT_MAX, MAX_DEPTH, &line);
    std::cout << "main: val: " << val << '\n';

    Move *move = line.moves[0];
    std::cout << "main: move: " << move->to_string() << '\n';
    board.make_move(move);
  }
}