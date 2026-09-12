#include "stargaze/tt.hpp"
#include <algorithm>
#include <bit>

TT::TT(size_t megabytes) : table_size(0), current_age(0) { resize(megabytes); }

void TT::resize(size_t megabytes) {
    const size_t bytes = std::max<size_t>(1, megabytes) * 1024 * 1024;
    const size_t new_size =
        std::bit_floor(std::max<size_t>(1, bytes / sizeof(TTEntry)));
    std::vector<TTEntry> new_table(new_size);
    table.swap(new_table);
    table_size = new_size;
    current_age = 0;
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

bool TT::should_replace(TTEntry *entry, uint64_t hash, uint8_t halfmove_clock,
                        uint8_t depth) const {
    return entry->bound == Bound::NONE ||
           (entry->hash == hash && entry->halfmove_clock != halfmove_clock) ||
           depth >= entry->depth || (current_age - entry->age) > 2;
}

void TT::store(uint64_t hash, Move best_move, Score score, uint8_t depth,
               uint8_t halfmove_clock, Bound bound) {
    size_t index = hash & (table_size - 1);
    TTEntry *entry = &table[index];

    if (should_replace(entry, hash, halfmove_clock, depth)) {
        entry->hash = hash;
        entry->best_move = best_move;
        entry->score = score;
        entry->depth = depth;
        entry->halfmove_clock = halfmove_clock;
        entry->bound = bound;
        entry->age = current_age;
    }
}
