#include "search.h"
#include <iostream>

int Search::alpha_beta(int alpha, int beta, int depth_left) {
  if (depth_left == 0) return quisce(alpha, beta);
  int best_value = -INT_MAX;
  std::vector<Move *> moves = board.get_moves();
  for (Move *move : moves) {
    board.make_move(move);
    int score = -alpha_beta(-beta, -alpha, depth_left - 1);
    board.undo_move();
    if (score >= beta) break; // fail soft beta-cutoff
    if (score > best_value) {
      best_value = score;
      if (score > alpha) {
        principle_variation.emplace_back(move);
        alpha = score;
      }
    }
  }
  return best_value;
}

int Search::quisce(int alpha, int beta) { // quiescence search
  int stand_pat = board.evaluate();
  if (stand_pat >= beta) return beta;
  if (alpha < stand_pat) alpha = stand_pat;
  std::vector<Move *> moves = board.get_moves();
  for (Move *move : moves) {
    if (!move->is_capture()) continue;
    board.make_move(move);
    int score = -quisce(-beta, -alpha);
    board.undo_move();
    if (score >= beta) return beta;
    if (score > alpha) alpha = score;
  }
  return alpha;
}