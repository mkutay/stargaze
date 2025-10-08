#include "tt.hpp"

void TT::clear() {
    table.clear();
    current_age = 0;
}

void TT::new_search() {
    current_age++;
}

std::optional<TTEntry *> TT::probe(u_int64_t hash) {
    if (table.count(hash) == 0) {
        return std::nullopt;
    }

    return &table[hash];
}

void TT::store(u_int64_t hash, Move best_move, int score, int depth, Bound bound) {
    if (table.count(hash) == 0) {
        TTEntry *entry = new TTEntry(
            best_move, static_cast<int16_t>(score), static_cast<int8_t>(depth), static_cast<uint8_t>(bound), current_age
        );
        table[hash] = *entry;
        return;
    }

    TTEntry* entry = &table[hash];
    
    bool should_replace = 
        entry->bound == BOUND_NONE ||
        depth >= entry->depth ||
        (current_age - entry->age) > 2; // old search
    
    if (should_replace) {
        entry->best_move = best_move;
        entry->score = static_cast<int16_t>(score);
        entry->depth = static_cast<int8_t>(depth);
        entry->bound = static_cast<uint8_t>(bound);
        entry->age = current_age;
    }
}

size_t TT::get_num_entries() const { return table.size(); }