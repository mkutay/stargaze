#include "tt.hpp"

void TT::clear() {
    table.clear();
    current_age = 0;
}

void TT::new_search() { current_age++; }

std::optional<TTEntry *> TT::probe(uint64_t hash) {
    auto it = table.find(hash);
    if (it == table.end()) {
        return std::nullopt;
    }

    return &it->second;
}

void TT::store(uint64_t hash, PVLine line, int score, int8_t depth,
               Bound bound) {
    auto [it, inserted] =
        table.try_emplace(hash, line, score, depth, bound, current_age);

    if (inserted) {
        return;
    }

    TTEntry *entry = &it->second;

    bool should_replace = entry->bound == Bound::NONE ||
                          depth >= entry->depth ||
                          (current_age - entry->age) > 2; // old search

    if (should_replace) {
        entry->line = line;
        entry->score = score;
        entry->depth = depth;
        entry->bound = bound;
        entry->age = current_age;
    }
}

size_t TT::get_num_entries() const { return table.size(); }
