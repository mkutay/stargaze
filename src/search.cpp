#include "search.hpp"
#include "evaluate.hpp"
#include <iostream>

Search::Search(Board *board) { this->board = board; }

int Search::alpha_beta(int alpha, int beta, int depth_left, PVLine *pline) {
  if (depth_left == 0) return quiescence(alpha, beta);
  PVLine line(depth_left - 1);

  std::vector<u_int16_t> moves = board->get_moves();
  for (u_int16_t move : moves) {
    board->make_move(move);
    int score = -alpha_beta(-beta, -alpha, depth_left - 1, &line);
    board->undo_move();

    if (score > alpha) {
      alpha = score;
      pline->moves[0] = move;
      std::copy(line.moves.begin(), line.moves.end(), pline->moves.begin() + 1);
    }
    if (score >= beta) break; // fail soft beta-cutoff
  }
  return alpha;
}

int Search::quiescence(int alpha, int beta) { // quiescence search
  int stand_pat = evaluate(*board);
  if (stand_pat >= beta) return beta;
  alpha = std::max(alpha, stand_pat);
  std::vector<u_int16_t> moves = board->get_moves();
  for (u_int16_t move : moves) {
    if (!is_move_capture(move)) continue;
    board->make_move(move);
    int score = -quiescence(-beta, -alpha);
    board->undo_move();
    if (score >= beta) return beta;
    alpha = std::max(alpha, score);
  }
  return alpha;
}