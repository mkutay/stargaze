#include "stargaze/tt.hpp"
#include <algorithm>
#include <bit>

TT::TT(size_t megabytes) : cluster_count(0), current_age(0) {
    resize(megabytes);
}

void TT::resize(size_t megabytes) {
    const size_t size = std::bit_floor(
        std::max(1zu, megabytes * 1024 * 1024 / sizeof(TTCluster)));
    std::vector<TTCluster> new_table(size);
    table.swap(new_table);
    cluster_count = size;
    current_age = 0;
}

void TT::clear() {
    std::fill(table.begin(), table.end(), TTCluster());
    current_age = 0;
}

void TT::new_search() { current_age++; }

TTEntry *TT::probe(uint64_t hash) {
    TTCluster &cluster = table[index(hash)];
    for (TTEntry &entry : cluster.entries) {
        if (entry.bound != Bound::NONE && entry.hash == hash) {
            entry.age = current_age;
            return &entry;
        }
    }
    return nullptr;
}

void TT::prefetch(uint64_t hash) const {
#if defined(__clang__) || defined(__GNUC__)
    __builtin_prefetch(&table[index(hash)]);
#else
    (void) hash;
#endif
}

int TT::replacement_quality(const TTEntry &entry) const {
    const uint32_t age = current_age - entry.age;
    return static_cast<int>(entry.depth) +
           (entry.bound == Bound::EXACT ? 2 : 0) - 4 * age;
}

void TT::store(uint64_t hash, std::optional<Move> best_move, Score score,
               uint8_t depth, uint8_t halfmove_clock, Bound bound) {
    TTCluster &cluster = table[index(hash)];
    TTEntry *victim = nullptr;

    for (TTEntry &entry : cluster.entries) {
        if (entry.bound == Bound::NONE) {
            victim = &entry;
            break;
        }

        if (entry.hash == hash) {
            if (entry.halfmove_clock != halfmove_clock ||
                depth >= entry.depth ||
                (bound == Bound::EXACT && depth + 2 >= entry.depth)) {
                victim = &entry;
            } else {
                entry.age = current_age;
                if (best_move)
                    entry.best_move = best_move;
            }
            break;
        }

        if (victim == nullptr ||
            replacement_quality(entry) < replacement_quality(*victim))
            victim = &entry;
    }

    const int incoming_quality = depth + (bound == Bound::EXACT ? 2 : 0);
    if (victim == nullptr ||
        (victim->bound != Bound::NONE && victim->hash != hash &&
         incoming_quality < replacement_quality(*victim)))
        return;

    *victim = TTEntry{hash,  score,          current_age, best_move,
                      depth, halfmove_clock, bound};
}

int TT::hashfull() const {
    constexpr size_t MAX_CLUSTERS = 2000 / TTCluster::SIZE;
    const size_t clusters = std::min<size_t>(MAX_CLUSTERS, cluster_count);
    size_t used = 0;
    for (size_t i = 0; i < clusters; i++)
        for (const TTEntry &entry : table[i].entries)
            used += entry.bound != Bound::NONE && entry.age == current_age;
    return static_cast<int>(used * MAX_CLUSTERS / clusters);
}
