#pragma once
#include <vector>
#include <cstring>
#include "move.hpp"

enum Bound {
    BOUND_NONE = 0,
    BOUND_EXACT = 1,  // PV-node (exact score)
    BOUND_LOWER = 2,  // All-node (failed high, beta cutoff)
    BOUND_UPPER = 3   // Cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    u_int64_t key;      // Zobrist hash key for verification
    Move best_move;     // Best move found in this position
    int16_t score;      // Score (evaluation)
    int8_t depth;       // Search depth
    uint8_t bound;      // Bound type (BOUND_EXACT, BOUND_LOWER, BOUND_UPPER)
    uint8_t age;        // Age counter for replacement strategy
    
    TTEntry() : key(0), best_move(), score(0), depth(0), bound(BOUND_NONE), age(0) {}
};

class TT {
private:
    std::vector<TTEntry> table;
    size_t size_mb;
    size_t num_entries;
    uint8_t current_age;
    
    inline size_t get_index(u_int64_t hash) const;
    
public:
    TT() : size_mb(0), num_entries(0), current_age(0) {}
    
    void resize(size_t mb);
    void clear();
    void new_search();
    bool probe(u_int64_t hash, TTEntry*& entry);
    void store(u_int64_t hash, Move best_move, int score, int depth, Bound bound);
    int get_occupancy() const;
    size_t get_size() const;
    size_t get_num_entries() const;
};