#pragma once
#include "move.hpp"
#include <vector>

struct PVLine {
    std::vector<Move> moves;
    PVLine(int max_depth) { moves.reserve(max_depth); }
    PVLine() {}
};
