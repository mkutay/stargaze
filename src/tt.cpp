#include "tt.hpp"

inline size_t TT::get_index(u_int64_t hash) const {
    return hash % num_entries;
}

void TT::resize(size_t mb) {
    size_mb = mb;
    num_entries = (mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(num_entries);
    clear();
}

void TT::clear() {
    std::memset(table.data(), 0, num_entries * sizeof(TTEntry));
    current_age = 0;
}

void TT::new_search() {
    current_age++;
    if (current_age == 0) current_age = 1; // avoid wraparound to 0
}

bool TT::probe(u_int64_t hash, TTEntry*& entry) {
    if (num_entries == 0) return false;
    
    size_t index = get_index(hash);
    entry = &table[index];
    
    // verify the key matches
    return entry->key == hash && entry->bound != BOUND_NONE;
}

void TT::store(u_int64_t hash, Move best_move, int score, int depth, Bound bound) {
    if (num_entries == 0) return;
    
    size_t index = get_index(hash);
    TTEntry* entry = &table[index];
    
    bool should_replace = 
        entry->bound == BOUND_NONE ||
        entry->key == hash ||
        depth >= entry->depth ||
        (current_age - entry->age) > 2; // old search
    
    if (should_replace) {
        entry->key = hash;
        entry->best_move = best_move;
        entry->score = static_cast<int16_t>(score);
        entry->depth = static_cast<int8_t>(depth);
        entry->bound = static_cast<uint8_t>(bound);
        entry->age = current_age;
    }
}

int TT::get_occupancy() const {
    if (num_entries == 0) return 0;
    
    int filled = 0;
    for (size_t i = 0; i < num_entries && i < 1000; i++) {
        if (table[i].bound != BOUND_NONE) filled++;
    }
    return (filled * 1000) / std::min(num_entries, size_t(1000));
}

size_t TT::get_size() const { return size_mb; }
size_t TT::get_num_entries() const { return num_entries; }