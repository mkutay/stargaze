#include "search.hpp"
#include "evaluate.hpp"
#include <iostream>

int Search::alpha_beta(int alpha, int beta, int depth_left, PVLine *pline) {
  // u_int64_t hash = board->get_hash();
  // if (tt.is_hash_present(hash)) {
  //   Entry entry = tt.get_entry(hash);
  //   if (entry.depth >= depth_left) {
  //     if (entry.score >= beta) return beta;
  //     if (entry.score > alpha) alpha = entry.score;
  //   }
  // }
  if (depth_left == 0) return quiescence(alpha, beta);
  PVLine line(depth_left - 1);

  std::vector<u_int16_t> moves = board->get_moves();
  for (u_int16_t move : moves) {
    board->make_move(move);
    // std::cout << board->to_string() << std::endl;
    int score = -alpha_beta(-beta, -alpha, depth_left - 1, &line);
    board->undo_move();

    if (score > alpha) {
      alpha = score;
      pline->moves[0] = move;
      std::copy(line.moves.begin(), line.moves.end(), pline->moves.begin() + 1);
    }
    if (score >= beta) break;
  }
  // Entry entry = { hash, depth_left, alpha };
  // tt.set_entry(hash, entry);
  return alpha;
}

int Search::quiescence(int alpha, int beta) { // quiescence search
  int stand_pat = evaluate(*board);
  int best_value = stand_pat;
  if (stand_pat >= beta) return stand_pat;
  if (alpha < stand_pat) alpha = stand_pat;
  std::vector<u_int16_t> moves = board->get_moves();
  for (u_int16_t move : moves) {
    if (!is_move_capture(move)) continue;
    board->make_move(move);
    // std::cout << "QUIESCENCE\n" << board->to_string() << std::endl;
    int score = -quiescence(-beta, -alpha);
    board->undo_move();
    if (score >= beta) return score;
    if (score > best_value) best_value = score;
    if (score > alpha) alpha = score;
  }
  return best_value;
}