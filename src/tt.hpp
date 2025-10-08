#pragma once
#include <vector>
#include <cstring>
#include <unordered_map>
#include <optional>
#include "move.hpp"

enum Bound {
    BOUND_NONE = 0,
    BOUND_EXACT = 1, // PV-node (exact score)
    BOUND_LOWER = 2, // All-node (failed high, beta cutoff)
    BOUND_UPPER = 3  // Cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    Move best_move; // Best move found in this position
    int16_t score;  // Score (evaluation)
    int8_t depth;   // Search depth
    u_int8_t bound; // Bound type (BOUND_EXACT, BOUND_LOWER, BOUND_UPPER)
    u_int8_t age;   // Age counter for replacement strategy
    
    TTEntry() : best_move(), score(0), depth(0), bound(BOUND_NONE), age(0) {}
    TTEntry(Move move, int16_t score, int8_t depth, u_int8_t bound, u_int8_t age)
        : best_move(move), score(score), depth(depth), bound(bound), age(age) {}
};

class TT {
    std::unordered_map<u_int64_t, TTEntry> table;
    u_int32_t current_age;
public:
    TT() : current_age(0) {}
    void clear();
    void new_search();
    std::optional<TTEntry *> probe(u_int64_t hash);
    void store(u_int64_t hash, Move best_move, int score, int depth, Bound bound);
    size_t get_num_entries() const;
};