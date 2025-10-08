#pragma once
#include <vector>
#include <chrono>
#include <algorithm>
#include "board.hpp"
#include "tt.hpp"
#include "move.hpp"

struct SearchInfo {
    int depth;
    int score;
    int nodes;
    long long time_ms;
    PVLine pv;
    bool stopped;
    SearchInfo(int max_depth) : depth(0), score(0), nodes(0), time_ms(0), pv(max_depth), stopped(false) {}
};

class Search {
private:
    Board *board;
    TT tt;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    long long time_limit_ms;
    int nodes_searched;
    bool time_up;
    
    int quiescence(int alpha, int beta, int depth);
    bool should_stop();
    void order_moves(std::vector<Move>& moves, const PVLine *pv_line, const PVLine *tt_line);
public:
    Search(Board *board) : board(board), time_limit_ms(5000), nodes_searched(0), time_up(false) {}
    
    int alpha_beta(int alpha, int beta, int depth_left, PVLine *pline);
    SearchInfo iterative_deepening(int max_depth, long long time_limit = 5000, SearchInfo *last_info = nullptr);
    void set_time_limit(long long ms) { time_limit_ms = ms; }
    const TT *get_tt() { return &tt; }
};