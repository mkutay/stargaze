#pragma once
#include <vector>
#include "board.hpp"
#include "tt.hpp"
#include "move.hpp"

typedef struct PVLine {
    std::vector<Move> moves;
    PVLine(int max_depth) : moves(max_depth) {}
} PVLine;

class Search {
private:
    Board *board;
    TT tt;
    int quiescence(int alpha, int beta, int depth);
public:
    Search(Board *board) { this->board = board; }
    int alpha_beta(int alpha, int beta, int depth_left, PVLine *pline);
};