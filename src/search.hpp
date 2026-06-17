#pragma once
#include "board.hpp"
#include "move.hpp"
#include "pv.hpp"
#include "tt.hpp"
#include <array>
#include <chrono>
#include <vector>

struct SearchInfo {
    bool stopped;
    uint16_t depth;
    uint32_t nodes;
    uint32_t time_ms;
    int score;
    PVLine pv;
    SearchInfo(int max_depth)
        : stopped(false), depth(0), nodes(0), time_ms(0), score(0),
          pv(max_depth) {}
};

class Search {
  private:
    constexpr static const int ALPHA_START = -100000;
    constexpr static const int BETA_START = 100000;
    constexpr static const int CHECKMATE_SCORE = -200000;
    constexpr static const int ASPIRATION_WINDOW = 23; // ~0.25 pawn

    // If the delta becomes too large in iterative deepening, reset alpha/beta
    // to the initial starting value to avoid searching too far below the
    // expected score.
    constexpr static const int DELTA_MAX =
        ASPIRATION_WINDOW * 4 * 5; // ~5 pawns

    bool time_up;
    uint32_t time_limit_ms;
    uint32_t nodes_searched;
    Board *board;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    TT tt;

    std::vector<std::array<Move, 2>> killers;

    int quiescence(int alpha, int beta);

    /**
     * Check if the search should stop due to time limit or other conditions.
     * Used for iterative deepening and alpha-beta search to terminate early if
     * needed.
     */
    bool should_stop();

    /**
     * Score a move to improve alpha-beta pruning efficiency.
     */
    int score_move(Move move, std::optional<Move> tt_move,
                   std::optional<uint16_t> ply);

    /**
     * Order the moves to improve alpha-beta pruning efficiency.
     */
    void order_moves(std::vector<Move> &moves, std::optional<Move> tt_move,
                     std::optional<uint16_t> ply);

    /**
     * Perform alpha-beta search with the given alpha, beta, depth left, and
     * ply. The PV line is updated with the best moves found during the search.
     */
    int alpha_beta(int alpha, int beta, uint16_t depth_left, uint16_t ply,
                   PVLine *pline);

  public:
    Search(Board *board) : board(board) {}

    /**
     * Perform iterative deepening search up to the given maximum depth and time
     * limit. The last search info can be provided to continue from a previous
     * search.
     */
    SearchInfo iterative_deepening(uint16_t max_depth, uint32_t time_limit);
};
