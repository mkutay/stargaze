#pragma once
#include "move.hpp"
#include <cstdint>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <vector>

struct PVLine {
    std::vector<Move> moves;
    PVLine(int max_depth) { moves.reserve(max_depth); }
    PVLine() {}
};

enum class Bound : uint8_t {
    NONE = 0,
    EXACT = 1, // PV-node (exact score)
    LOWER = 2, // all-node (failed high, beta cutoff)
    UPPER = 3  // cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    int score;    // score (evaluation)
    int8_t depth; // search depth
    uint8_t age;  // age counter for replacement strategy
    Bound bound;  // bound type (BOUND_EXACT, BOUND_LOWER, BOUND_UPPER)
    PVLine line;

    TTEntry() : score(0), depth(0), age(0), bound(Bound::NONE), line() {}
    TTEntry(PVLine line_, int score_, int8_t depth_, Bound bound_, uint8_t age_)
        : score(score_), depth(depth_), age(age_), bound(bound_), line(line_) {}
};

class TT {
    std::unordered_map<uint64_t, TTEntry> table;
    uint32_t current_age;

  public:
    TT() : current_age(0) {}
    void clear();
    void new_search();
    std::optional<TTEntry *> probe(uint64_t hash);
    void store(uint64_t hash, PVLine line, int score, int8_t depth,
               Bound bound);
    size_t get_num_entries() const;
};
