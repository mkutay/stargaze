#pragma once
#include <vector>
#include <cstring>
#include <unordered_map>
#include <optional>
#include "move.hpp"

struct PVLine {
    std::vector<Move> moves;
    PVLine(int max_depth) : moves(max_depth) {}
    PVLine() {}
};

enum class Bound {
    NONE = 0,
    EXACT = 1, // PV-node (exact score)
    LOWER = 2, // all-node (failed high, beta cutoff)
    UPPER = 3  // cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    int16_t score; // score (evaluation)
    int8_t depth; // search depth
    Bound bound; // bound type (BOUND_EXACT, BOUND_LOWER, BOUND_UPPER)
    u_int8_t age; // age counter for replacement strategy
    PVLine line;
    
    TTEntry() : score(0), depth(0), bound(Bound::NONE), age(0), line() {}
    TTEntry(PVLine line_, int16_t score_, int8_t depth_, Bound bound_, u_int8_t age_)
        : score(score_), depth(depth_), bound(bound_), age(age_), line(line_) {}
};

class TT {
    std::unordered_map<u_int64_t, TTEntry> table;
    u_int32_t current_age;
public:
    TT() : current_age(0) {}
    void clear();
    void new_search();
    std::optional<TTEntry *> probe(u_int64_t hash);
    void store(u_int64_t hash, PVLine line, int score, int depth, Bound bound);
    size_t get_num_entries() const;
};