#include "tt.hpp"
#include <algorithm>

TT::TT(size_t exp_size) : current_age(0) {
    table_size = 1uz << exp_size;
    table.resize(table_size);
}

void TT::clear() {
    std::fill(table.begin(), table.end(), TTEntry());
    current_age = 0;
}

void TT::new_search() { current_age++; }

TTEntry *TT::probe(uint64_t hash) {
    size_t index = hash & (table_size - 1);
    if (table[index].bound != Bound::NONE && table[index].hash == hash) {
        return &table[index];
    }
    return nullptr;
}

bool TT::should_replace(TTEntry *entry, uint8_t depth) const {
    return entry->bound == Bound::NONE || depth >= entry->depth ||
           (current_age - entry->age) > 2;
}

void TT::store(uint64_t hash, Move best_move, int score, uint8_t depth,
               Bound bound) {
    size_t index = hash & (table_size - 1);
    TTEntry *entry = &table[index];

    if (should_replace(entry, depth)) {
        entry->hash = hash;
        entry->best_move = best_move;
        entry->score = score;
        entry->depth = depth;
        entry->bound = bound;
        entry->age = current_age;
    }
}
