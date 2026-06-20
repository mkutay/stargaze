#pragma once
#include "move.hpp"
#include "score.hpp"
#include <cstdint>
#include <vector>

enum class Bound : uint8_t {
    NONE = 0,  // entry is empty/invalid
    EXACT = 1, // PV-node (exact score)
    LOWER = 2, // all-node (failed high, beta cutoff)
    UPPER = 3  // cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    uint64_t hash;
    Score score;
    Move best_move;
    uint8_t depth;
    Bound bound;
    uint16_t age;

    TTEntry()
        : hash(0), score(0), best_move(0), depth(0), bound(Bound::NONE),
          age(0) {}
};

class TT {
  private:
    std::vector<TTEntry> table;
    size_t table_size;
    uint16_t current_age;

    bool should_replace(TTEntry *entry, uint8_t depth) const;

  public:
    TT(size_t exp_size = 24);
    void clear();
    void new_search();
    TTEntry *probe(uint64_t hash);
    void store(uint64_t hash, Move best_move, Score score, uint8_t depth,
               Bound bound);
};
