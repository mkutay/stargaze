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
    Bound bound;    // bound type (BOUND_EXACT, BOUND_LOWER, BOUND_UPPER)
    uint16_t depth; // search depth
    uint16_t age;   // age counter for replacement strategy
    int score;      // score (evaluation)
    PVLine line;

    TTEntry() : bound(Bound::NONE), depth(0), age(0), score(0), line() {}
    TTEntry(PVLine line_, int score_, uint16_t depth_, Bound bound_,
            uint16_t age_)
        : bound(bound_), depth(depth_), age(age_), score(score_), line(line_) {}
};

class TT {
    std::unordered_map<uint64_t, TTEntry> table;

    /**
     * Age counter for the transposition table. This is incremented at the start
     * of each search, and is used to determine whether an entry is "old" and
     * should be replaced. Maximum it could get is the number of half-moves in a
     * game.
     */
    uint16_t current_age;

  public:
    TT() : current_age(0) {}
    void clear();
    void new_search();
    std::optional<TTEntry *> probe(uint64_t hash);
    void store(uint64_t hash, PVLine line, int score, uint16_t depth,
               Bound bound);
    size_t get_num_entries() const;
};
