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

void TT::store(u_int64_t hash, PVLine line, int score, int depth, Bound bound) {
    if (table.count(hash) == 0) {
        table[hash] = TTEntry(
            line, static_cast<int16_t>(score), static_cast<int8_t>(depth), bound, current_age
        );
        return;
    }

    TTEntry* entry = &table[hash];
    
    bool should_replace = 
        entry->bound == Bound::NONE ||
        depth >= entry->depth ||
        (current_age - entry->age) > 2; // old search
    
    if (should_replace) {
        entry->line = line;
        entry->score = static_cast<int16_t>(score);
        entry->depth = static_cast<int8_t>(depth);
        entry->bound = bound;
        entry->age = current_age;
    }
}

size_t TT::get_num_entries() const { return table.size(); }