#pragma once
#include "board.hpp"
#include "move.hpp"
#include "pv.hpp"
#include "tt.hpp"
#include <chrono>
#include <optional>
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
    bool time_up;
    uint32_t time_limit_ms;
    uint32_t nodes_searched;
    Board *board;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    TT tt;

    /**
     * Perform quiescence search with the given alpha, beta, and depth. This is
     * used to search only capture moves and avoid the horizon effect.
     */
    int quiescence(int alpha, int beta, uint16_t depth);

    /**
     * Check if the search should stop due to time limit or other conditions.
     * Used for iterative deepening and alpha-beta search to terminate early if
     * needed.
     */
    bool should_stop();

    /**
     * Order the moves to improve alpha-beta pruning efficiency.
     */
    void order_moves(std::vector<Move> &moves, const PVLine *pv_line,
                     const PVLine *tt_line);

  public:
    Search(Board *board) : board(board) {}

    /**
     * Perform alpha-beta search with the given alpha, beta, and depth left. The
     * PV line is updated with the best moves found during the search.
     */
    int alpha_beta(int alpha, int beta, uint16_t depth_left, PVLine *pline);

    /**
     * Perform iterative deepening search up to the given maximum depth and time
     * limit. The last search info can be provided to continue from a previous
     * search.
     */
    SearchInfo iterative_deepening(uint16_t max_depth, uint32_t time_limit,
                                   std::optional<SearchInfo> last_info);
};
