#include <iostream>
#include <random>
#include "board.h"

typedef std::mt19937 MyRNG;

int32_t main() {
  Board board;
  std::cout << board.to_string() << std::endl;
  // for (Move *move : moves) {
  //   std::cout << move->get_from() << " " << move->get_to() << " " << move->get_flags() << std::endl;
  // }
  MyRNG rng;
  std::uniform_int_distribution<uint32_t> rand;
  for (int i = 0; i < 50; i++) {
    std::vector<Move *> moves = board.get_moves();
    Move * random_move = moves[rand(rng) % moves.size()];
    board.make_move(random_move);
    std::cout << board.to_string() << std::endl;
  }
}