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
    uint64_t nodes;
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

    // Named constants for move scoring
    constexpr static const int TT_MOVE_SCORE = 100000;
    constexpr static const int PROMOTION_SCORE = 90000;
    constexpr static const int CAPTURE_SCORE_BASE = 70000;
    constexpr static const int CASTLE_SCORE = 60000;
    constexpr static const int KILLER_SCORE_1 = 50000;
    constexpr static const int KILLER_SCORE_2 = 40000;

    bool time_up;
    uint32_t time_limit_ms;
    uint64_t nodes_searched;
    Board *board;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    TT tt;

    // Killers are a heuristic to improve move ordering in alpha-beta search.
    // The idea is that if a move causes a beta cutoff at a certain depth, it is
    // likely to be a good move in similar positions. We store the two most
    // recent killer moves for each ply (depth) of the search.
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
    int score_move(Move move, Move tt_move, uint16_t ply);
    int score_move(Move move);
    int get_capture_value(Move move) const;

    /**
     * Order the moves to improve alpha-beta pruning efficiency.
     */
    void order_moves(std::vector<Move> &moves, Move tt_move, uint16_t ply);
    void order_moves(std::vector<Move> &moves);

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
    SearchInfo iterative_deepening(uint16_t max_depth, uint32_t _time_limit_ms);
};
