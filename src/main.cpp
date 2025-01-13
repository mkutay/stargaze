#include <iostream>
#include <random>
#include <stack>
#include "search.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "get_hash.hpp"

int MAX_DEPTH = 4;

int32_t main() {
  init_eval_table();
  init_hash_table();
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

    u_int16_t move = line.moves[0];
    std::cout << "main: move: " << move_to_string(move) << '\n';
    board.make_move(move);
  }
}